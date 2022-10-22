//
// weproxy@foxmail.com 2022/10/22
//

use std::{
    cell::UnsafeCell,
    cmp::min,
    collections::VecDeque,
    io,
    net::SocketAddr,
    ops::{Deref, DerefMut},
    os::raw,
    pin::Pin,
    sync::atomic::{AtomicBool, Ordering},
    sync::Arc,
};

use anyhow::Result;
use bytes::BytesMut;
use futures::{
    stream::Stream,
    task::{Context, Poll, Waker},
};

use tokio::{
    io::{AsyncRead, AsyncWrite, ReadBuf},
    sync::mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender},
};

use super::c::*;
use super::util;
use crate::fx::*;
use log::*;

///////////////////////////////////////////////////////////////////////////////////////////////////

pub use super::_tcp_cb::*;
pub use super::_tcp_ctx::*;
pub use super::_tcp_lis::*;

///////////////////////////////////////////////////////////////////////////////////////////////////

// TCP ...
pub struct TCP {
    lwip_lock: Arc<AtomicMutex>,
    src_addr: SocketAddr,
    dest_addr: SocketAddr,
    pcb: *mut tcp_pcb,
    write_buf: BytesMut,
    callback_ctx: TCPContext,
}

// TCP impl ...
impl TCP {
    // new ...
    pub fn new(lwip_lock: Arc<AtomicMutex>, pcb: *mut tcp_pcb) -> Result<Box<Self>> {
        unsafe {
            // Since we have no idea how to deal with a full bounded channel upon receiving
            // data from lwIP, an unbounded channel is used instead.
            //
            // Note that lwIP is in charge of flow control. If reader is slower than writer,
            // lwIP will propagate the pressure back by announcing a decreased window size.
            // Thus our unbounded channel will never be overwhelmed. To achieve this, we must
            // call `tcp_recved` when the data from our internal buffer are consumed.
            let (read_tx, read_rx) = unbounded_channel();
            let src_addr = util::to_socket_addr(&(*pcb).remote_ip, (*pcb).remote_port)?;
            let dest_addr = util::to_socket_addr(&(*pcb).local_ip, (*pcb).local_port)?;
            let stream = Box::new(TCP {
                lwip_lock,
                src_addr,
                dest_addr,
                pcb,
                write_buf: BytesMut::new(),
                callback_ctx: TCPContext::new(src_addr, dest_addr, read_tx, read_rx),
            });
            let arg = &stream.callback_ctx as *const _;
            tcp_arg(pcb, arg as *mut raw::c_void);
            tcp_recv(pcb, Some(tcp_recv_cb));
            tcp_sent(pcb, Some(tcp_sent_cb));
            tcp_err(pcb, Some(tcp_err_cb));

            stream.apply_pcb_opts();

            trace!("netstack tcp new {}", stream.local_addr());

            Ok(stream)
        }
    }

    #[cfg(not(target_os = "ios"))]
    fn apply_pcb_opts(&self) {}

    #[cfg(target_os = "ios")]
    fn apply_pcb_opts(&self) {
        unsafe { (*self.pcb).so_options |= SOF_KEEPALIVE as u8 };
    }

    pub fn local_addr(&self) -> &SocketAddr {
        &self.src_addr
    }

    pub fn remote_addr(&self) -> &SocketAddr {
        &self.dest_addr
    }
}

// broken_pipe ...
fn broken_pipe() -> io::Error {
    io::Error::new(io::ErrorKind::BrokenPipe, "broken pipe")
}

// TCP AsyncRead ...
impl AsyncRead for TCP {
    // poll_read ...
    fn poll_read(
        mut self: Pin<&mut Self>,
        cx: &mut Context,
        buf: &mut ReadBuf,
    ) -> Poll<io::Result<()>> {
        let me = &mut *self;
        let guard = me.lwip_lock.lock();
        let ContextInner {
            ref mut read_rx,
            errored,
            ..
        } = *me.callback_ctx.with_lock(&guard);
        // handle any previously overflowed data
        if !me.write_buf.is_empty() {
            let to_read = min(buf.remaining(), me.write_buf.len());
            let piece = me.write_buf.split_to(to_read);
            buf.put_slice(&piece[..to_read]);
            unsafe { tcp_recved(me.pcb, to_read as u16_t) };
            return Poll::Ready(Ok(()));
        }
        match Pin::new(read_rx).poll_recv(cx) {
            Poll::Ready(Some(data)) => {
                let to_read = min(buf.remaining(), data.len());
                buf.put_slice(&data[..to_read]);
                if to_read < data.len() {
                    // overflow
                    me.write_buf.extend_from_slice(&data[to_read..]);
                }
                unsafe { tcp_recved(me.pcb, to_read as u16_t) };
                Poll::Ready(Ok(()))
            }
            Poll::Ready(None) => {
                Poll::Ready(Ok(())) // eof
            }
            // no more buffered data
            Poll::Pending => {
                // report error after all buffered/overflowed data are handled
                if errored {
                    Poll::Ready(Err(broken_pipe()))
                } else {
                    Poll::Pending
                }
            }
        }
    }
}

// TCP Sync/Send ...
unsafe impl Sync for TCP {}
unsafe impl Send for TCP {}

// TCP Drop ...
impl Drop for TCP {
    fn drop(&mut self) {
        let guard = self.lwip_lock.lock();
        let ContextInner {
            local_addr,
            errored,
            ..
        } = *self.callback_ctx.with_lock(&guard);
        trace!("netstack tcp drop {}", local_addr);
        if !errored {
            unsafe {
                tcp_arg(self.pcb, std::ptr::null_mut());
                tcp_recv(self.pcb, None);
                tcp_sent(self.pcb, None);
                tcp_err(self.pcb, None);
                tcp_close(self.pcb);
            }
        }
    }
}

// TCP AsyncWrite ...
impl AsyncWrite for TCP {
    // poll_write ...
    fn poll_write(self: Pin<&mut Self>, cx: &mut Context, buf: &[u8]) -> Poll<io::Result<usize>> {
        let guard = self.lwip_lock.lock();
        let ContextInner {
            ref mut write_waker,
            errored,
            ..
        } = *self.callback_ctx.with_lock(&guard);
        if errored {
            return Poll::Ready(Err(broken_pipe()));
        }
        let to_write = min(buf.len(), unsafe { (*self.pcb).snd_buf as usize });
        if to_write == 0 {
            if write_waker
                .as_ref()
                .map(|w| !w.will_wake(cx.waker()))
                .unwrap_or(true)
            {
                write_waker.replace(cx.waker().clone());
            }
            return Poll::Pending;
        }
        let err = unsafe {
            tcp_write(
                self.pcb,
                buf.as_ptr() as *const raw::c_void,
                to_write as u16_t,
                TCP_WRITE_FLAG_COPY as u8,
            )
        };
        if err == err_enum_t_ERR_OK as err_t {
            let err = unsafe { tcp_output(self.pcb) };
            if err == err_enum_t_ERR_OK as err_t {
                Poll::Ready(Ok(to_write))
            } else {
                Poll::Ready(Err(io::Error::new(
                    io::ErrorKind::Interrupted,
                    format!("netstack tcp_output error {}", err),
                )))
            }
        } else if err == err_enum_t_ERR_MEM as err_t {
            warn!("netstack tcp err_mem");
            if write_waker
                .as_ref()
                .map(|w| !w.will_wake(cx.waker()))
                .unwrap_or(true)
            {
                write_waker.replace(cx.waker().clone());
            }
            Poll::Pending
        } else {
            Poll::Ready(Err(io::Error::new(
                io::ErrorKind::Interrupted,
                format!("netstack tcp_write error {}", err),
            )))
        }
    }

    // poll_flush ...
    fn poll_flush(self: Pin<&mut Self>, _cx: &mut Context) -> Poll<io::Result<()>> {
        let guard = self.lwip_lock.lock();
        if self.callback_ctx.with_lock(&guard).errored {
            return Poll::Ready(Err(broken_pipe()));
        }
        let err = unsafe { tcp_output(self.pcb) };
        if err != err_enum_t_ERR_OK as err_t {
            Poll::Ready(Err(io::Error::new(
                io::ErrorKind::Interrupted,
                format!("netstack tcp_output error {}", err),
            )))
        } else {
            Poll::Ready(Ok(()))
        }
    }

    // poll_shutdown ...
    fn poll_shutdown(self: Pin<&mut Self>, _cx: &mut Context) -> Poll<io::Result<()>> {
        let guard = self.lwip_lock.lock();
        let ContextInner {
            local_addr,
            errored,
            ..
        } = *self.callback_ctx.with_lock(&guard);
        if errored {
            return Poll::Ready(Err(broken_pipe()));
        }
        trace!("netstack tcp shutdown {}", local_addr);
        let err = unsafe { tcp_shutdown(self.pcb, 0, 1) };
        if err != err_enum_t_ERR_OK as err_t {
            Poll::Ready(Err(io::Error::new(
                io::ErrorKind::Interrupted,
                format!("netstack tcp_shutdown tx error {}", err),
            )))
        } else {
            Poll::Ready(Ok(()))
        }
    }
}

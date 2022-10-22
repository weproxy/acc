//
// weproxy@foxmail.com 2022/10/22
//

use std::{
    collections::VecDeque,
    fmt,
    net::SocketAddr,
    os::raw,
    pin::Pin,
    sync::{Arc, Mutex},
};

use futures::{
    stream::Stream,
    task::{Context, Poll, Waker},
};
use crate::fx::*;
use supper::c::*;
use log::*;

// Listener ...
pub struct Listener {
    pub tpcb: *mut tcp_pcb,
    pub lwip_lock: Arc<AtomicMutex>,
    pub waker: Option<Waker>,
    pub queue: VecDeque<Box<TCP>>,
}

// Listener impl ...
impl Listener {
    pub fn new(lwip_lock: Arc<AtomicMutex>) -> Box<Self> {
        unsafe {
            let _g = lwip_lock.lock();
            let mut tpcb = tcp_new();
            let err = tcp_bind(tpcb, &ip_addr_any_type, 0);
            if err != err_enum_t_ERR_OK as err_t {
                error!("bind tcp failed");
                panic!("");
            }
            let mut reason: err_t = 0;
            tpcb = tcp_listen_with_backlog_and_err(
                tpcb,
                TCP_DEFAULT_LISTEN_BACKLOG as u8,
                &mut reason,
            );
            if tpcb.is_null() {
                error!("listen tcp failed: {}", reason);
                panic!("");
            }
            let listener = Box::new(Listener {
                tpcb,
                lwip_lock: lwip_lock.clone(),
                waker: None,
                queue: VecDeque::new(),
            });
            let arg = &*listener as *const Listener as *mut raw::c_void;
            tcp_arg(listener.tpcb, arg);
            tcp_accept(listener.tpcb, Some(tcp_accept_cb));
            listener
        }
    }
}

// Listener Sync/Send ...
unsafe impl Sync for Listener {}
unsafe impl Send for Listener {}

// Listener Drop ...
impl Drop for Listener {
    fn drop(&mut self) {
        unsafe {
            let _g = self.lwip_lock.lock();
            tcp_accept(self.tpcb, None);
            tcp_close(self.tpcb);
        }
    }
}

// Listener Stream ...
impl Stream for Listener {
    type Item = Box<TCP>;

    // poll_next ...
    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context) -> Poll<Option<Self::Item>> {
        if let Some(stream) = self.queue.pop_front() {
            return Poll::Ready(Some(stream));
        }
        if let Some(waker) = self.waker.as_ref() {
            if !waker.will_wake(cx.waker()) {
                self.waker.replace(cx.waker().clone());
            }
        } else {
            self.waker.replace(cx.waker().clone());
        }
        Poll::Pending
    }
}

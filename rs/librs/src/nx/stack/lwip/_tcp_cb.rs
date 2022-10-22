//
// weproxy@foxmail.com 2022/10/22
//

use super::c::*;
use std::os::raw;
use log::*;

use super::_tcp_ctx::*;

///////////////////////////////////////////////////////////////////////////////////////////////////

#[allow(unused_variables)]
pub extern "C" fn tcp_accept_cb(arg: *mut raw::c_void, newpcb: *mut tcp_pcb, err: err_t) -> err_t {
    if newpcb.is_null() {
        warn!("tcp full");
        return err_enum_t_ERR_OK as err_t;
    }
    let listener = unsafe { &mut *(arg as *mut super::tcp::Listener) };
    let stream = match super::tcp::TCP::new(listener.lwip_lock.clone(), newpcb) {
        Ok(s) => s,
        Err(e) => {
            error!("new tcp stream failed: {}", e);
            return err_enum_t_ERR_OK as err_t;
        }
    };
    listener.queue.push_back(stream);
    if let Some(waker) = listener.waker.as_ref() {
        waker.wake_by_ref();
    }
    err_enum_t_ERR_OK as err_t
}

// tcp_recv_cb ...
#[allow(unused_variables)]
pub extern "C" fn tcp_recv_cb(
    arg: *mut raw::c_void,
    tpcb: *mut tcp_pcb,
    p: *mut pbuf,
    err: err_t,
) -> err_t {
    // SAFETY: tcp_recv_cb is called from tcp_input or sys_check_timeouts only when
    // a data packet or previously refused data is received. Thus lwip_lock must be locked.
    // See also `<NetStackImpl as AsyncWrite>::poll_write`.
    let ContextInner {
        local_addr,
        remote_addr,
        ref mut read_tx,
        ..
    } = *unsafe { TCPContext::assume_locked(arg as *const TCPContext) };

    if p.is_null() {
        trace!("netstack tcp eof {}", local_addr);
        let _ = read_tx.take();
        return err_enum_t_ERR_OK as err_t;
    }

    let pbuflen = unsafe { (*p).tot_len };
    let buflen = pbuflen as usize;
    let mut buf = Vec::<u8>::with_capacity(buflen);
    unsafe {
        pbuf_copy_partial(p, buf.as_mut_ptr() as _, pbuflen, 0);
        buf.set_len(pbuflen as usize);
    };

    if let Some(Err(err)) = read_tx.as_ref().map(|tx| tx.send(buf)) {
        // rx is closed
        // TODO remove this message
        trace!(
            "netstack tcp recv {} bytes data on {} -> {} failed: {}",
            pbuflen,
            local_addr,
            remote_addr,
            err
        );
        return unsafe { tcp_shutdown(tpcb, 1, 0) };
    }

    unsafe { pbuf_free(p) };
    err_enum_t_ERR_OK as err_t
}

// tcp_sent_cb ...
#[allow(unused_variables)]
pub extern "C" fn tcp_sent_cb(arg: *mut raw::c_void, tpcb: *mut tcp_pcb, len: u16_t) -> err_t {
    // SAFETY: tcp_sent_cb is called from tcp_input only when
    // an ACK packet is received. Thus lwip_lock must be locked.
    // See also `<NetStackImpl as AsyncWrite>::poll_write`.
    let ctx = &*unsafe { TCPContext::assume_locked(arg as *const TCPContext) };
    if let Some(waker) = ctx.write_waker.as_ref() {
        waker.wake_by_ref();
    }
    err_enum_t_ERR_OK as err_t
}

// tcp_err_cb ...
#[allow(unused_variables)]
pub extern "C" fn tcp_err_cb(arg: *mut ::std::os::raw::c_void, err: err_t) {
    // SAFETY: tcp_err_cb is called from
    // tcp_input, tcp_abandon, tcp_abort, tcp_alloc and tcp_new.
    // Thus lwip_lock must be locked before calling any of these.
    let ContextInner {
        local_addr,
        read_tx,
        errored,
        ..
    } = &mut *unsafe { TCPContext::assume_locked(arg as *const TCPContext) };
    trace!("netstack tcp err {} {}", err, local_addr);
    *errored = true;
    let _ = read_tx.take();
}

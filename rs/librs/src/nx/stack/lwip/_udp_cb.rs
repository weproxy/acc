//
// weproxy@foxmail.com 2022/10/22
//

// use super::c::*;
// use std::os::raw;
// use log::*;

// use super::_tcp_lis::*;
// use super::util;

// use crate::nx::socks;

///////////////////////////////////////////////////////////////////////////////////////////////////

// udp_recv_cb ...
pub extern "C" fn udp_recv_cb(
    arg: *mut raw::c_void,
    _pcb: *mut udp_pcb,
    p: *mut pbuf,
    addr: *const ip_addr_t,
    port: u16_t,
    dst_addr: *const ip_addr_t,
    dst_port: u16_t,
) {
    let listener = unsafe { &mut *(arg as *mut Listener) };
    let src_addr = unsafe {
        match util::to_socket_addr(&*addr, port) {
            Ok(a) => a,
            Err(e) => {
                warn!("udp recv failed: {}", e);
                return;
            }
        }
    };
    let dst_addr = unsafe {
        match util::to_socket_addr(&*dst_addr, dst_port) {
            Ok(a) => a,
            Err(e) => {
                warn!("udp recv failed: {}", e);
                return;
            }
        }
    };

    let tot_len = unsafe { (*p).tot_len };
    let n = tot_len as usize;
    let mut buf = Vec::<u8>::with_capacity(n);
    unsafe {
        pbuf_copy_partial(p, buf.as_mut_ptr() as *mut raw::c_void, tot_len, 0);
        buf.set_len(n);
        pbuf_free(p);
    }

    match listener.queue.lock() {
        Ok(mut queue) => {
            let pkt = super::udp::Packet {
                data: (&buf[..n]).to_vec(),
                src_addr: Some(socks::SocksAddr::Ip(src_addr)),
                dst_addr: Some(socks::SocksAddr::Ip(dst_addr)),
            };
            queue.push_back(pkt);
            match listener.waker.lock() {
                Ok(waker) => {
                    if let Some(waker) = waker.as_ref() {
                        waker.wake_by_ref();
                    }
                }
                Err(err) => {
                    error!("udp waker lock waker failed {:?}", err);
                }
            }
        }
        Err(err) => {
            error!("udp listener lock queue failed {:?}", err);
        }
    }
}

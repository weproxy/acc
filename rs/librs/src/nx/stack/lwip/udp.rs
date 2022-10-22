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

use log::*;

use super::c::*;
use super::util;
use crate::fx::*;
use crate::nx::socks;

///////////////////////////////////////////////////////////////////////////////////////////////////

// include!("_udp_cb.rs");
// include!("_udp_lis.rs");

///////////////////////////////////////////////////////////////////////////////////////////////////

// Packet ...
#[derive(Debug)]
pub struct Packet {
    pub data: Vec<u8>,
    pub src_addr: Option<socks::SocksAddr>,
    pub dst_addr: Option<socks::SocksAddr>,
}

// Packet Display ...
impl std::fmt::Display for Packet {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let src = match self.src_addr {
            None => "None".to_string(),
            Some(ref addr) => addr.to_string(),
        };
        let dst = match self.dst_addr {
            None => "None".to_string(),
            Some(ref addr) => addr.to_string(),
        };
        write!(f, "{} <-> {}, {} bytes", src, dst, self.data.len())
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////

// StreamId ...
pub type StreamId = u64;

///////////////////////////////////////////////////////////////////////////////////////////////////

// Source ...
#[derive(PartialEq, Eq, Hash, Clone, Copy, Debug)]
pub struct Source {
    pub address: SocketAddr,
    pub stream_id: Option<StreamId>,
}

// Source impl ...
impl Source {
    pub fn new(address: SocketAddr, stream_id: Option<StreamId>) -> Self {
        Source { address, stream_id }
    }
}

// Source Display ...
impl std::fmt::Display for Source {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        if let Some(id) = self.stream_id.as_ref() {
            write!(f, "{}(stream-{})", self.address, id)
        } else {
            write!(f, "{}", self.address)
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// sendto ...
#[allow(unused)]
pub fn sendto(
    lwip_lock: Arc<AtomicMutex>,
    src_addr: &SocketAddr,
    dst_addr: &SocketAddr,
    pcb: usize,
    data: &[u8],
) {
    unsafe {
        let _g = lwip_lock.lock();
        let pbuf = pbuf_alloc_reference(
            data as *const [u8] as *mut [u8] as *mut raw::c_void,
            data.len() as u16_t,
            pbuf_type_PBUF_ROM,
        );
        let src_ip = match util::to_ip_addr_t(&src_addr.ip()) {
            Ok(v) => v,
            Err(e) => {
                warn!("convert ip failed: {}", e);
                return;
            }
        };
        let dst_ip = match util::to_ip_addr_t(&dst_addr.ip()) {
            Ok(v) => v,
            Err(e) => {
                warn!("convert ip failed: {}", e);
                return;
            }
        };
        let err = udp_sendto(
            pcb as *mut udp_pcb,
            pbuf,
            &dst_ip as *const ip_addr_t,
            dst_addr.port() as u16_t,
            &src_ip as *const ip_addr_t,
            src_addr.port() as u16_t,
        );
        if err != err_enum_t_ERR_OK as err_t {
            warn!("udp_sendto err {}", err);
        }
        pbuf_free(pbuf);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// UDP ...
pub struct UDP {
    // pub lwip_lock: Arc<AtomicMutex>,
}

// UDP impl ...
impl UDP {
    // new ...
    pub fn new() -> Self {
        UDP {}
    }
}

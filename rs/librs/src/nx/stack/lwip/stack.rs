//
// weproxy@foxmail.com 2022/10/22
//

use std::{
    io,
    os::raw,
    pin::Pin,
    sync::{
        mpsc::{self, Receiver, Sender},
        Arc, Once,
    },
    time,
};

use futures::{
    stream::StreamExt,
    task::{Context, Poll, Waker},
};

use tokio::{
    self,
    io::{AsyncRead, AsyncWrite, ReadBuf},
    sync::{
        mpsc::{channel as TokioChannel, Receiver as TokioReceiver, Sender as TokioSender},
    },
};

use super::c::*;
use crate::fx::*;
use crate::nx::socks;

use log::*;

use super::output::*;
use super::tcp;
use super::udp;

///////////////////////////////////////////////////////////////////////////////////////////////////

// LWIP_INIT ...
static LWIP_INIT: Once = Once::new();

///////////////////////////////////////////////////////////////////////////////////////////////////

// Stack ...
pub struct Stack {
    pub lwip_lock: Arc<AtomicMutex>,
    waker: Option<Waker>,
    tx: Sender<Vec<u8>>,
    rx: Receiver<Vec<u8>>,
}

// Stack impl ...
impl Stack {
    // new ...
    pub fn new() -> Box<Self> {
        LWIP_INIT.call_once(|| unsafe { lwip_init() });

        unsafe {
            (*netif_list).output = Some(output_ip4);
            (*netif_list).output_ip6 = Some(output_ip6);
            (*netif_list).mtu = 1500;
        }

        let (tx, rx): (Sender<Vec<u8>>, Receiver<Vec<u8>>) = mpsc::channel();

        let stack = Box::new(Stack {
            lwip_lock: Arc::new(AtomicMutex::new()),
            waker: None,
            tx,
            rx,
            // dispatcher,
            // nat_manager,
            // fakedns,
        });

        unsafe {
            OUTPUT_CB_PTR = &*stack as *const Stack as usize;
        }

        // init_tcp ...
        stack.init_tcp();

        // init_udp ...
        stack.init_udp();

        // loop_check_timeout
        stack.loop_check_timeout();

        stack
    }

    // init_tcp ...
    fn init_tcp(&self) {
        // let inbound_tag_1 = inbound_tag.clone();
        let lwip_lock = self.lwip_lock.clone();
        // let dispatcher = stack.dispatcher.clone();
        // let fakedns = stack.fakedns.clone();
        tokio::spawn(async move {
            let mut listener = tcp::Listener::new(lwip_lock);

            while let Some(stream) = listener.next().await {
                // let dispatcher = dispatcher.clone();
                // let fakedns = fakedns.clone();
                // let inbound_tag_1 = inbound_tag_1.clone();

                tokio::spawn(async move {
                    // let mut sess = Session {
                    //     network: Network::TCP,
                    //     source: stream.local_addr().to_owned(),
                    //     local_addr: stream.remote_addr().to_owned(),
                    //     destination: socks::SocksAddr::Ip(*stream.remote_addr()),
                    //     inbound_tag: inbound_tag_1.clone(),
                    //     ..Default::default()
                    // };

                    // if fakedns.lock().await.is_fake_ip(&stream.remote_addr().ip()) {
                    //     if let Some(domain) = fakedns
                    //         .lock()
                    //         .await
                    //         .query_domain(&stream.remote_addr().ip())
                    //     {
                    //         sess.destination =
                    //             socks::SocksAddr::Domain(domain, stream.remote_addr().port());
                    //     } else {
                    //         // Although requests targeting fake IPs are assumed
                    //         // never happen in real network traffic, which are
                    //         // likely caused by poisoned DNS cache records, we
                    //         // still have a chance to sniff the request domain
                    //         // for TLS traffic in dispatcher.
                    //         if stream.remote_addr().port() != 443 {
                    //             return;
                    //         }
                    //     }
                    // }

                    // dispatcher
                    //     .dispatch_tcp(&mut sess, TcpStream::new(stream))
                    //     .await;
                });
            }
        });
    }
    // init_udp ...
    fn init_udp(&self) {
        let lwip_lock = self.lwip_lock.clone();
        // let nat_manager = self.nat_manager.clone();
        // let fakedns = self.fakedns.clone();
        tokio::spawn(async move {
            let mut listener = udp::Listener::new();
            // let nat_manager = nat_manager.clone();
            // let fakedns2 = fakedns.clone();
            let pcb = listener.pcb();

            // Sending packets to TUN should be very fast.
            let (client_ch_tx, mut client_ch_rx): (
                TokioSender<udp::Packet>,
                TokioReceiver<udp::Packet>,
            ) = TokioChannel(32);

            // downlink
            let lwip_lock2 = lwip_lock.clone();
            tokio::spawn(async move {
                while let Some(pkt) = client_ch_rx.recv().await {
                    let socks_src_addr = match pkt.src_addr {
                        Some(a) => a,
                        None => {
                            warn!("unexpected none src addr");
                            continue;
                        }
                    };
                    let dst_addr = match pkt.dst_addr {
                        Some(a) => match a {
                            socks::SocksAddr::Ip(a) => a,
                            _ => {
                                warn!("unexpected domain addr");
                                continue;
                            }
                        },
                        None => {
                            warn!("unexpected dst addr");
                            continue;
                        }
                    };
                    let src_addr = match socks_src_addr {
                        socks::SocksAddr::Ip(a) => a,

                        // If the socket gives us a domain source address,
                        // we assume there must be a paired fake IP, otherwise
                        // we have no idea how to deal with it.
                        socks::SocksAddr::Domain(domain, port) => {
                            // TODO we're doing this for every packet! optimize needed
                            // trace!("downlink querying fake ip for domain {}", &domain);
                            // if let Some(ip) = fakedns2.lock().await.query_fake_ip(&domain) {
                            //     SocketAddr::new(ip, port)
                            // } else {
                                warn!(
                                    "unexpected domain src addr {}:{} without paired fake IP",
                                    &domain, &port
                                );
                                continue;
                            // }
                        },
                    };
                    udp::sendto(lwip_lock2.clone(), &src_addr, &dst_addr, pcb, &pkt.data[..]);
                }

                error!("unexpected udp downlink ended");
            });

            // let fakedns2 = fakedns.clone();

            while let Some(pkt) = listener.next().await {
                let src_addr = match pkt.src_addr {
                    Some(a) => match a {
                        socks::SocksAddr::Ip(a) => a,
                        _ => {
                            warn!("unexpected domain addr");
                            continue;
                        }
                    },
                    None => {
                        warn!("unexpected none src addr");
                        continue;
                    }
                };
                let dst_addr = match pkt.dst_addr {
                    Some(a) => match a {
                        socks::SocksAddr::Ip(a) => a,
                        _ => {
                            warn!("unexpected domain addr");
                            continue;
                        }
                    },
                    None => {
                        warn!("unexpected dst addr");
                        continue;
                    }
                };

                // if dst_addr.port() == 53 {
                //     match fakedns2.lock().await.generate_fake_response(&pkt.data) {
                //         Ok(resp) => {
                //             send_udp(lwip_lock.clone(), &dst_addr, &src_addr, pcb, resp.as_ref());
                //             continue;
                //         }
                //         Err(err) => {
                //             trace!("generate fake ip failed: {}", err);
                //         }
                //     }
                // }

                // We're sending UDP packets to a fake IP, and there should be a paired domain,
                // that said, the application connects a UDP socket with a domain address.
                // It also means the back packets on this UDP session shall only come from a
                // single source address.
                // let socks_dst_addr = if fakedns2.lock().await.is_fake_ip(&dst_addr.ip()) {
                //     // TODO we're doing this for every packet! optimize needed
                //     // trace!("uplink querying domain for fake ip {}", &dst_addr.ip(),);
                //     if let Some(domain) = fakedns2.lock().await.query_domain(&dst_addr.ip()) {
                //         socks::SocksAddr::Domain(domain, dst_addr.port())
                //     } else {
                //         // Skip this packet. Requests targeting fake IPs are
                //         // assumed never happen in real network traffic.
                //         continue;
                //     }
                // } else {
                //     socks::SocksAddr::Ip(dst_addr)
                // };
                let socks_dst_addr = socks::SocksAddr::Ip(dst_addr);

                let dgram_src = udp::Source::new(src_addr, None);

                let pkt = udp::Packet {
                    data: pkt.data,
                    src_addr: Some(socks::SocksAddr::Ip(dgram_src.address)),
                    dst_addr: Some(socks_dst_addr.clone()),
                };

                // nat_manager
                //     .send(&dgram_src, socks_dst_addr, &inbound_tag, pkt, &client_ch_tx)
                //     .await;
            }
        });
    }

    // loop_check_timeout ...
    fn loop_check_timeout(&self) {
        let lwip_lock = self.lwip_lock.clone();
        tokio::spawn(async move {
            loop {
                {
                    let _g = lwip_lock.lock();
                    unsafe { sys_check_timeouts() };
                }
                tokio::time::sleep(time::Duration::from_millis(250)).await;
            }
        });
    }

    // output ...
    pub fn output(&mut self, pkt: Vec<u8>) -> io::Result<usize> {
        let n = pkt.len();
        if let Err(err) = self.tx.send(pkt) {
            println!("output packet failed: {}", err);
            return Ok(0);
        }
        if let Some(waker) = self.waker.as_ref() {
            waker.wake_by_ref();
            return Ok(n);
        }
        Ok(0)
    }
}

// Stack Drop ...
impl Drop for Stack {
    fn drop(&mut self) {
        log::trace!("drop netstack");
        unsafe {
            let _g = self.lwip_lock.lock();
            OUTPUT_CB_PTR = 0x0;
        };
    }
}

// Stack AsyncRead ...
impl AsyncRead for Stack {
    // poll_read ...
    fn poll_read(
        mut self: Pin<&mut Self>,
        cx: &mut Context,
        buf: &mut ReadBuf,
    ) -> Poll<io::Result<()>> {
        match self.rx.try_recv() {
            Ok(pkt) => {
                if pkt.len() > buf.remaining() {
                    warn!("truncated pkt, short buf");
                }
                buf.put_slice(&pkt);
                Poll::Ready(Ok(()))
            }
            Err(_) => {
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
    }
}

// Stack AsyncWrite ...
impl AsyncWrite for Stack {
    // poll_write ...
    fn poll_write(self: Pin<&mut Self>, _cx: &mut Context, buf: &[u8]) -> Poll<io::Result<usize>> {
        unsafe {
            let _g = self.lwip_lock.lock();

            let pbuf = pbuf_alloc(pbuf_layer_PBUF_RAW, buf.len() as u16_t, pbuf_type_PBUF_RAM);
            if pbuf.is_null() {
                warn!("alloc null pbuf");
                return Poll::Pending;
            }
            pbuf_take(pbuf, buf.as_ptr() as *const raw::c_void, buf.len() as u16_t);

            if let Some(input_fn) = (*netif_list).input {
                let err = input_fn(pbuf, netif_list);
                if err == err_enum_t_ERR_OK as err_t {
                    Poll::Ready(Ok(buf.len()))
                } else {
                    pbuf_free(pbuf);
                    Poll::Ready(Err(io::Error::new(
                        io::ErrorKind::Interrupted,
                        "input failed",
                    )))
                }
            } else {
                pbuf_free(pbuf);
                Poll::Ready(Err(io::Error::new(
                    io::ErrorKind::Interrupted,
                    "none input fn",
                )))
            }
        }
    }

    // poll_flush ...
    fn poll_flush(self: Pin<&mut Self>, _cx: &mut Context) -> Poll<io::Result<()>> {
        Poll::Ready(Ok(()))
    }

    // poll_shutdown ...
    fn poll_shutdown(self: Pin<&mut Self>, _cx: &mut Context) -> Poll<io::Result<()>> {
        Poll::Ready(Ok(()))
    }
}

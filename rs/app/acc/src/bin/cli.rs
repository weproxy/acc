use smoltcp::phy::wait as phy_wait;
use smoltcp::phy::{Device, RawSocket, RxToken};
use smoltcp::time::Instant;
use smoltcp::wire::{EthernetFrame, PrettyPrinter};
use std::env;
use std::os::unix::io::AsRawFd;
use log::*;

fn main() {
    // info!("hello");
    println!("hello!");
    // let ifname = env::args().nth(1).unwrap();
    // let mut socket = RawSocket::new(ifname.as_ref(), smoltcp::phy::Medium::Ethernet).unwrap();
    // loop {
    //     phy_wait(socket.as_raw_fd(), None).unwrap();
    //     let (rx_token, _) = socket.receive().unwrap();
    //     rx_token
    //         .consume(Instant::now(), |buffer| {
    //             println!(
    //                 "{}",
    //                 PrettyPrinter::<EthernetFrame<&[u8]>>::new("", &buffer)
    //             );
    //             Ok(())
    //         })
    //         .unwrap();
    // }
}

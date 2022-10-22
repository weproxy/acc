//
// weproxy@foxmail.com 2022/10/22
//

use std::{io, pin::Pin};

use futures::task::{Context, Poll};
use tokio::io::{AsyncRead, AsyncWrite, ReadBuf};

use super::lwip;
pub use super::lwip::Stack;

// // Stack ...
// pub struct Stack {
//     inner: Box<lwip::Stack>,
// }

// // Stack ...
// impl Stack {
//     // new ...
//     pub fn new() -> Self {
//         Stack { inner: lwip::Stack::new() }
//     }

//     // output ...
//     pub fn output(&mut self, pkt: Vec<u8>) -> io::Result<usize> {
//         self.inner.output(pkt)
//     }
// }

// // Stack AsyncRead ...
// impl AsyncRead for Stack {
//     // poll_read ...
//     fn poll_read(
//         mut self: Pin<&mut Self>,
//         cx: &mut Context,
//         buf: &mut ReadBuf,
//     ) -> Poll<io::Result<()>> {
//         AsyncRead::poll_read(Pin::new(&mut self.inner), cx, buf)
//     }
// }

// // Stack AsyncWrite ...
// impl AsyncWrite for Stack {
//     // poll_write ...
//     fn poll_write(
//         mut self: Pin<&mut Self>,
//         cx: &mut Context,
//         buf: &[u8],
//     ) -> Poll<io::Result<usize>> {
//         AsyncWrite::poll_write(Pin::new(&mut self.inner), cx, buf)
//     }

//     // poll_flush ...
//     fn poll_flush(mut self: Pin<&mut Self>, cx: &mut Context) -> Poll<io::Result<()>> {
//         AsyncWrite::poll_flush(Pin::new(&mut self.inner), cx)
//     }

//     // poll_shutdown ...
//     fn poll_shutdown(mut self: Pin<&mut Self>, cx: &mut Context) -> Poll<io::Result<()>> {
//         AsyncWrite::poll_shutdown(Pin::new(&mut self.inner), cx)
//     }
// }

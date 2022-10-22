//
// weproxy@foxmail.com 2022/10/22
//

use std::{pin::Pin};
use std::io;

use futures::task::{Context, Poll};
use tokio::io::{AsyncRead, AsyncWrite, ReadBuf};

use super::lwip;

///////////////////////////////////////////////////////////////////////////////////////////////////

// TCP ...
pub struct TCP {
    inner: Box<lwip::TCP>,
}

// TCP impl ...
impl TCP {
    // new ...
    pub fn new(p: Box<lwip::TCP>) -> Self {
        TCP { inner: p }
    }
}

// TCP AsyncRead ...
impl AsyncRead for TCP {
    fn poll_read(
        mut self: Pin<&mut Self>,
        cx: &mut Context,
        buf: &mut ReadBuf,
    ) -> Poll<io::Result<()>> {
        AsyncRead::poll_read(Pin::new(&mut self.inner), cx, buf)
    }
}

// TCP AsyncWrite ...
impl AsyncWrite for TCP {
    fn poll_write(
        mut self: Pin<&mut Self>,
        cx: &mut Context,
        buf: &[u8],
    ) -> Poll<io::Result<usize>> {
        AsyncWrite::poll_write(Pin::new(&mut self.inner), cx, buf)
    }

    fn poll_flush(mut self: Pin<&mut Self>, cx: &mut Context) -> Poll<io::Result<()>> {
        AsyncWrite::poll_flush(Pin::new(&mut self.inner), cx)
    }

    fn poll_shutdown(mut self: Pin<&mut Self>, cx: &mut Context) -> Poll<io::Result<()>> {
        AsyncWrite::poll_shutdown(Pin::new(&mut self.inner), cx)
    }
}

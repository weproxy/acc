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

// ContextInner ...
pub struct ContextInner {
    pub local_addr: SocketAddr,
    pub remote_addr: SocketAddr,
    pub read_tx: Option<UnboundedSender<Vec<u8>>>,
    pub read_rx: UnboundedReceiver<Vec<u8>>,
    pub errored: bool,
    pub write_waker: Option<Waker>,
}

// ContextRef ...
#[repr(transparent)]
pub struct ContextRef<'a> {
    ctx: &'a TCPContext,
}

// ContextRef Deref ...
impl<'a> Deref for ContextRef<'a> {
    type Target = ContextInner;
    fn deref(&self) -> &Self::Target {
        unsafe { &*self.ctx.inner.get() }
    }
}

// ContextRef DerefMut ...
impl<'a> DerefMut for ContextRef<'a> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { &mut *self.ctx.inner.get() }
    }
}

// ContextRef Drop ...
impl<'a> Drop for ContextRef<'a> {
    fn drop(&mut self) {
        self.ctx.borrowed.store(false, Ordering::Release);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

/// Context shared by TCP and lwIP callbacks.
pub struct TCPContext {
    inner: UnsafeCell<ContextInner>,
    borrowed: AtomicBool,
}

// Users must hold a lwip_lock to get the mutable reference to inner data,
// or go through unsafe interfaces.
unsafe impl Sync for TCPContext {}

// TCPContext impl ...
impl TCPContext {
    pub fn new(
        local_addr: SocketAddr,
        remote_addr: SocketAddr,
        read_tx: UnboundedSender<Vec<u8>>,
        read_rx: UnboundedReceiver<Vec<u8>>,
    ) -> Self {
        TCPContext {
            inner: UnsafeCell::new(ContextInner {
                local_addr,
                remote_addr,
                read_tx: Some(read_tx),
                read_rx,
                errored: false,
                write_waker: None,
            }),
            borrowed: AtomicBool::new(false),
        }
    }

    /// Access to inner data with lwip_lock locked.
    ///
    /// # Panics
    ///
    /// Panics if another reference to inner data exists.
    pub fn with_lock<'a>(&'a self, _guard: &'a AtomicMutexGuard) -> ContextRef<'a> {
        if self.borrowed.swap(true, Ordering::Acquire) {
            panic!("TCPContext locked twice within a locked period")
        }
        ContextRef { ctx: self }
    }

    /// Access to inner data within a lwIP callback where lwip_lock is guaranteed to be locked.
    ///
    /// # Panics
    ///
    /// Panics if another reference to inner data exists.
    pub unsafe fn assume_locked<'a>(ptr: *const Self) -> ContextRef<'a> {
        ContextRef { ctx: &*ptr }
    }
}

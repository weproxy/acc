//
// weproxy@foxmail.com 2022/10/22
//

use std::sync::atomic::{AtomicBool, Ordering::*};

 // AtomicMutex ...
#[derive(Debug)]
pub struct AtomicMutex {
    locked: AtomicBool,
}

// AtomicMutexErr ...
#[derive(Debug, Clone, Copy)]
pub struct AtomicMutexErr;

// AtomicMutexGuard ...
pub struct AtomicMutexGuard<'a> {
    mutex: &'a AtomicMutex,
}

// AtomicMutex impl ...
impl AtomicMutex {
    // new ...
    pub fn new() -> Self {
        Self {
            locked: AtomicBool::new(false),
        }
    }

    // try_lock ...
    pub fn try_lock(&self) -> Result<AtomicMutexGuard<'_>, AtomicMutexErr> {
        if self.locked.swap(true, Acquire) {
            Err(AtomicMutexErr)
        } else {
            Ok(AtomicMutexGuard { mutex: self })
        }
    }

    // lock ...
    pub fn lock(&self) -> AtomicMutexGuard<'_> {
        loop {
            if let Ok(m) = self.try_lock() {
                break m;
            }
        }
    }
}

// AtomicMutex Default ...
impl Default for AtomicMutex {
    fn default() -> Self {
        Self::new()
    }
}

unsafe impl Send for AtomicMutex {}

unsafe impl Sync for AtomicMutex {}

// AtomicMutexGuard Drop ...
impl<'a> Drop for AtomicMutexGuard<'a> {
    fn drop(&mut self) {
        let _prev = self.mutex.locked.swap(false, Release);
        debug_assert!(_prev);
    }
}

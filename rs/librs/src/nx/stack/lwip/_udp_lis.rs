//
// weproxy@foxmail.com 2022/10/22
//

///////////////////////////////////////////////////////////////////////////////////////////////////

// Listener ...
pub struct Listener {
    pcb: *mut udp_pcb,
    pub waker: Arc<Mutex<Option<Waker>>>,
    pub queue: Arc<Mutex<VecDeque<Packet>>>,
}

// Listener impl ...
impl Listener {
    // new ...
    pub fn new() -> Self {
        unsafe {
            let pcb = udp_new();
            let listener = Listener {
                pcb,
                waker: Arc::new(Mutex::new(None)),
                queue: Arc::new(Mutex::new(VecDeque::new())),
            };
            let err = udp_bind(pcb, &ip_addr_any_type, 0);
            if err != err_enum_t_ERR_OK as err_t {
                error!("bind udp failed");
                panic!("");
            }
            let arg = &listener as *const Listener as *mut raw::c_void;
            udp_recv(pcb, Some(udp_recv_cb), arg);
            listener
        }
    }

    pub fn pcb(&self) -> usize {
        self.pcb as usize
    }
}

///////////////////////////////////////////////////////////////////////////////

unsafe impl Sync for Listener {}
unsafe impl Send for Listener {}

// Listener Drop ...
impl Drop for Listener {
    fn drop(&mut self) {
        unsafe {
            udp_recv(self.pcb, None, std::ptr::null_mut());
            udp_remove(self.pcb);
        }
    }
}

// Listener Stream ...
impl Stream for Listener {
    type Item = Packet;

    // poll_next ...
    fn poll_next(self: Pin<&mut Self>, cx: &mut Context) -> Poll<Option<Self::Item>> {
        match self.queue.lock() {
            Ok(mut queue) => {
                if let Some(sess) = queue.pop_front() {
                    return Poll::Ready(Some(sess));
                }
            }
            Err(err) => {
                error!("sess poll lock queue failed: {:?}", err);
            }
        }
        match self.waker.lock() {
            Ok(mut waker) => {
                if let Some(waker_ref) = waker.as_ref() {
                    if !waker_ref.will_wake(cx.waker()) {
                        waker.replace(cx.waker().clone());
                    }
                } else {
                    waker.replace(cx.waker().clone());
                }
            }
            Err(err) => {
                error!("sess poll lock waker failed: {:?}", err);
            }
        }
        Poll::Pending
    }
}

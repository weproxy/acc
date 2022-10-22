//
// weproxy@foxmail.com 2022/10/22
//

use super::c::*;
use super::Stack;

// OUTPUT_CB_PTR ...
pub static mut OUTPUT_CB_PTR: usize = 0x0;

// output ...
fn output(netif: *mut netif, p: *mut pbuf) -> err_t {
    unsafe {
        let pbuflen = (*p).tot_len;
        let mut buf = Vec::with_capacity((*netif).mtu as usize);
        pbuf_copy_partial(p, buf.as_mut_ptr() as *mut std::os::raw::c_void, pbuflen, 0);
        buf.set_len(pbuflen as usize);
        if OUTPUT_CB_PTR == 0x0 {
            return err_enum_t_ERR_ABRT as err_t;
        }
        let stack = &mut *(OUTPUT_CB_PTR as *mut Stack);
        let _ = stack.output((&buf[0..pbuflen as usize]).to_vec());
        err_enum_t_ERR_OK as err_t
    }
}

// output_ip4 ...
#[allow(unused_variables)]
pub extern "C" fn output_ip4(netif: *mut netif, p: *mut pbuf, ipaddr: *const ip4_addr_t) -> err_t {
    output(netif, p)
}

// output_ip6 ...
#[allow(unused_variables)]
#[allow(unused)]
pub extern "C" fn output_ip6(netif: *mut netif, p: *mut pbuf, ipaddr: *const ip6_addr_t) -> err_t {
    output(netif, p)
}

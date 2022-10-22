//
// weproxy@foxmail.com 2022/10/22
//

use super::lwip;

///////////////////////////////////////////////////////////////////////////////////////////////////

// UDP ...
pub struct UDP {
    inner: Box<lwip::UDP>,
}

// UDP impl ...
impl UDP {
    // new ...
    pub fn new(p: Box<lwip::UDP>) -> Self {
        UDP { inner: p }
    }
}

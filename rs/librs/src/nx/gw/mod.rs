//
// weproxy@foxmail.com 2022/10/22
//

mod net;
pub use net::*;

#[cfg(target_os = "macos")]
pub mod cmd_macos;
#[cfg(target_os = "macos")]
pub use cmd_macos as cmd;

#[cfg(target_os = "linux")]
pub mod cmd_linux;
#[cfg(target_os = "linux")]
pub use cmd_linux as cmd;

use anyhow::Result;

// get_gateway ...
pub fn get_gateway() -> Result<String> {
    Ok("".to_string())
}

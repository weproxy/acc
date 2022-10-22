//
// weproxy@foxmail.com 2022/10/22
//

#[allow(unused_imports)]
use anyhow::{self, bail, Result, Error};
// use anyhow::Error;
// use thiserror::Error;
use std::process::Command;

// get_ipv4_gateway ...
pub fn get_ipv4_gateway() -> Result<String> {
    let out = Command::new("route")
        .arg("-n")
        .arg("get")
        .arg("1")
        .output()
        .expect("failed to execute");

    assert!(out.status.success());

    let out = String::from_utf8_lossy(&out.stdout).to_string();

    let cols: Vec<&str> = out
        .lines()
        .find(|l| l.contains("gateway"))
        .unwrap()
        .split_whitespace()
        .map(str::trim)
        .collect();
    assert!(cols.len() == 2);
    // let res = cols[1].to_string();
    // Ok(res)
    // Err(anyhow!("not found"))
    bail!("not found")
}

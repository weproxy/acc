//
// weproxy@foxmail.com 2022/10/22
//

use std::{
    env,
    path::{Path, PathBuf},
    process::Command,
};

// compile_lwip ...
fn compile_lwip() {
    println!("cargo:rerun-if-changed=c");
    cc::Build::new()
        .file("src/nx/stack/lwip/c/core/init.c")
        .file("src/nx/stack/lwip/c/core/def.c")
        // .file("src/nx/stack/lwip/c/core/dns.c")
        .file("src/nx/stack/lwip/c/core/inet_chksum.c")
        .file("src/nx/stack/lwip/c/core/ip.c")
        .file("src/nx/stack/lwip/c/core/mem.c")
        .file("src/nx/stack/lwip/c/core/memp.c")
        .file("src/nx/stack/lwip/c/core/netif.c")
        .file("src/nx/stack/lwip/c/core/pbuf.c")
        .file("src/nx/stack/lwip/c/core/raw.c")
        // .file("src/nx/stack/lwip/c/core/stats.c")
        // .file("src/nx/stack/lwip/c/core/sys.c")
        .file("src/nx/stack/lwip/c/core/tcp.c")
        .file("src/nx/stack/lwip/c/core/tcp_in.c")
        .file("src/nx/stack/lwip/c/core/tcp_out.c")
        .file("src/nx/stack/lwip/c/core/timeouts.c")
        .file("src/nx/stack/lwip/c/core/udp.c")
        // .file("src/nx/stack/lwip/c/core/ipv4/autoip.c")
        // .file("src/nx/stack/lwip/c/core/ipv4/dhcp.c")
        // .file("src/nx/stack/lwip/c/core/ipv4/etharp.c")
        .file("src/nx/stack/lwip/c/core/ipv4/icmp.c")
        // .file("src/nx/stack/lwip/c/core/ipv4/igmp.c")
        .file("src/nx/stack/lwip/c/core/ipv4/ip4_frag.c")
        .file("src/nx/stack/lwip/c/core/ipv4/ip4.c")
        .file("src/nx/stack/lwip/c/core/ipv4/ip4_addr.c")
        // .file("src/nx/stack/lwip/c/core/ipv6/dhcp6.c")
        // .file("src/nx/stack/lwip/c/core/ipv6/ethip6.c")
        .file("src/nx/stack/lwip/c/core/ipv6/icmp6.c")
        // .file("src/nx/stack/lwip/c/core/ipv6/inet6.c")
        .file("src/nx/stack/lwip/c/core/ipv6/ip6.c")
        .file("src/nx/stack/lwip/c/core/ipv6/ip6_addr.c")
        .file("src/nx/stack/lwip/c/core/ipv6/ip6_frag.c")
        // .file("src/nx/stack/lwip/c/core/ipv6/mld6.c")
        .file("src/nx/stack/lwip/c/core/ipv6/nd6.c")
        .file("src/nx/stack/lwip/c/custom/sys_arch.c")
        .include("src/nx/stack/lwip/c/custom")
        .include("src/nx/stack/lwip/c/include")
        .warnings(false)
        .flag_if_supported("-Wno-everything")
        .compile("liblwip.a");
}

// generate_lwip_bindings ...
fn generate_lwip_bindings() {
    println!("cargo:rustc-link-lib=lwip");
    println!("cargo:rerun-if-changed=src/nx/stack/lwip/c_bindings.h");
    println!("cargo:include=src/nx/stack/lwip/c/include");

    let arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();
    let os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    let bindings = bindgen::Builder::default()
        .header("src/nx/stack/lwip/c_bindings.h")
        .clang_arg("-I./src/nx/stack/lwip/c/include")
        .clang_arg("-I./src/nx/stack/lwip/c/custom")
        .clang_arg("-Wno-everything")
        .layout_tests(false)
        .clang_arg(if arch == "aarch64" && os == "ios" {
            // https://github.com/rust-lang/rust-bindgen/issues/1211
            "--target=arm64-apple-ios"
        } else {
            ""
        })
        .clang_arg(if arch == "aarch64" && os == "ios" {
            // sdk path find by `xcrun --sdk iphoneos --show-sdk-path`
            let output = Command::new("xcrun")
                .arg("--sdk")
                .arg("iphoneos")
                .arg("--show-sdk-path")
                .output()
                .expect("failed to execute xcrun");
            let inc_path =
                Path::new(String::from_utf8_lossy(&output.stdout).trim()).join("usr/include");
            format!("-I{}", inc_path.to_str().expect("invalid include path"))
        } else {
            "".to_string()
        })
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let mut out_path = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    out_path = out_path.join("src/nx/stack/lwip");
    bindings
        .write_to_file(out_path.join("c_bindings.rs"))
        .expect("Couldn't write bindings!");
}

// generate_mobile_bindings ...
fn generate_mobile_bindings() {
    println!("cargo:rerun-if-changed=src/logx/mobile/wrapper.h");
    let arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();
    let os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    let bindings = bindgen::Builder::default()
        .header("src/logx/mobile/c_bindings.h")
        .clang_arg("-Wno-everything")
        .layout_tests(false)
        .clang_arg(if arch == "aarch64" && os == "ios" {
            // https://github.com/rust-lang/rust-bindgen/issues/1211
            "--target=arm64-apple-ios"
        } else {
            ""
        })
        .clang_arg(if arch == "aarch64" && os == "ios" {
            // sdk path find by `xcrun --sdk iphoneos --show-sdk-path`
            let output = Command::new("xcrun")
                .arg("--sdk")
                .arg("iphoneos")
                .arg("--show-sdk-path")
                .output()
                .expect("failed to execute xcrun");
            let inc_path =
                Path::new(String::from_utf8_lossy(&output.stdout).trim()).join("usr/include");
            format!("-I{}", inc_path.to_str().expect("invalid include path"))
        } else {
            "".to_string()
        })
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let mut out_path = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    out_path = out_path.join("src/logx/mobile");
    bindings
        .write_to_file(out_path.join("c_bindings.rs"))
        .expect("Couldn't write bindings!");
}

// main ...
fn main() {
    let os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    println!("[build] os={}", os);

    if os == "ios" || os == "android" || os == "linux" || os == "macos" {
        compile_lwip();
        generate_lwip_bindings();
    }

    if os == "ios" || os == "macos" || os == "android" {
        generate_mobile_bindings();
    }

    // if env::var("PROTO_GEN").is_ok() {
    //     // println!("cargo:rerun-if-changed=src/config/internal/config.proto");
    //     protoc_rust::Codegen::new()
    //         .out_dir("src/config/internal")
    //         .inputs(&["src/config/internal/config.proto"])
    //         .customize(protoc_rust::Customize {
    //             expose_oneof: Some(true),
    //             expose_fields: Some(true),
    //             generate_accessors: Some(false),
    //             lite_runtime: Some(true),
    //             ..Default::default()
    //         })
    //         .run()
    //         .expect("protoc");

    //     // println!("cargo:rerun-if-changed=src/config/geosite.proto");
    //     protoc_rust::Codegen::new()
    //         .out_dir("src/config")
    //         .inputs(&["src/config/geosite.proto"])
    //         .customize(protoc_rust::Customize {
    //             expose_oneof: Some(true),
    //             expose_fields: Some(true),
    //             generate_accessors: Some(false),
    //             lite_runtime: Some(true),
    //             ..Default::default()
    //         })
    //         .run()
    //         .expect("protoc");

    //     protoc_rust::Codegen::new()
    //         .out_dir("src/app/outbound")
    //         .inputs(&["src/app/outbound/selector_cache.proto"])
    //         .customize(protoc_rust::Customize {
    //             expose_oneof: Some(true),
    //             expose_fields: Some(true),
    //             generate_accessors: Some(false),
    //             lite_runtime: Some(true),
    //             ..Default::default()
    //         })
    //         .run()
    //         .expect("protoc");
    // }
}

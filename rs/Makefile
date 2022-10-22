#
# weproxy@foxmail.com 2022/10/22
#

.PHONY: check pre config all build run surun clean deploy

all: build

check:

pre: check

config: check

build: pre config
	cargo build --no-default-features

run: build
	cargo run --no-default-features

surun: build
	sudo ./target/debug/acc-cli

clean: config
	cargo clean

# cargo install cross
box:
#	cross build --release --target mipsel-unknown-linux-musl
	cross build --release --target mipsel-unknown-linux-musl --manifest-path librs/Cargo.toml  --no-default-features --features "default-openssl"
#	cargo build --release --target x86_64-unknown-linux-musl --no-default-features

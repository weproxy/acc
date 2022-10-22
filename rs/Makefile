#
# weproxy@foxmail.com 2022/10/22
#

.PHONY: check pre config all build run surun clean deploy

all: build

check:

pre: check

config: check

build: pre config
	cargo build

run: build
	cargo run

surun: build
	sudo ./target/debug/acc-cli

clean: config
	cargo clean

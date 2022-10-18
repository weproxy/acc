#
# weproxy@foxmail.com 2022/10/03
#

app ?= acc
cmd ?= cli
arg ?=
APP ?= ${app}
CMD ?= ${cmd}
ARG ?= ${arg}

env ?= debug
ENV ?= ${env}

ver ?= $(shell date "+%Y%m%d%H%M")
VER ?= ${ver}

ifeq (${APP},${CMD})
	NAME = ${APP}
else
	NAME = ${APP}-${CMD}
endif

ifneq ($(wildcard ./app/${NAME}),)
    SRCDIR = ./app/${NAME}
else
    SRCDIR = ./app/${app}/cmd/${cmd}
endif


.PHONY: pre all build run clean deploy

all: build

check:
	@if [ ! -e "${SRCDIR}" ]; then \
		echo "";\
		echo "ERROR: not found ${SRCDIR} "; \
		echo "";\
		exit -1; \
	fi

pre: check

config: check
	xmake config -m ${ENV} --with_libcurl=n --fpic=y

build: pre config
	xmake build

run: build
	xmake run

clean: config
	xmake clean

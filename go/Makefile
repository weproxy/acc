#
# weproxy@foxmail.com 2022/10/03
#

HOSTOS ?= $(shell echo `uname -s | tr A-Z a-z`)
HOSTARCH ?= $(shell echo `uname -m | tr A-Z a-z`)

GOOS ?= $(shell go env GOOS)
GOARCH ?= $(shell go env GOARCH)
GOARM ?= $(shell go env GOARM)

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

TAGS ?= ${tags}

BUILDMODE ?= ${buildmode}
ifeq ($(BUILDMODE),)
	BUILDMODE = default
endif


ifeq (${GOOS},darwin)
    EXT_LDFLAGS = -linkmode external
else
    EXT_LDFLAGS =
endif

ifeq (${APP},${CMD})
	NAME = ${APP}
else
	NAME = ${APP}-${CMD}
endif

ifeq ($(buildmode),c-shared)
	ifeq ($(GOOS),windows)
		BIN = ${NAME}.${GOARCH}.dll
	else ifeq ($(GOOS),darwin)
		BIN = ${NAME}.${GOARCH}.dylib
	else
		BIN = ${NAME}.${GOARCH}.so
	endif
else ifeq ($(buildmode),c-archive)
	ifeq ($(GOOS),windows)
		BIN = ${NAME}.${GOARCH}.lib
	else
		BIN = ${NAME}.${GOOS}.${GOARCH}.a
	endif
else
	ifeq ($(GOOS),windows)
		BIN = ${NAME}.${GOARCH}.exe
	else
		BIN = ${NAME}.${GOOS}.${GOARCH}
	endif
endif

ifneq ($(wildcard ./app/${NAME}),)
    SRCDIR = ./app/${NAME}/cmd
else
    SRCDIR = ./app/${app}/cmd/${cmd}
endif

OUTDIR = ./tmp/${NAME}

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
	@mkdir -p ${OUTDIR}

build: pre
	GOOS=${GOOS} GOARCH=${GOARCH} go build -tags=${TAGS} -trimpath -ldflags=" \
		-X 'webproxy/acc/libgo/biz.Build.Ver=${VER}' \
		-X 'webproxy/acc/libgo/biz.Build.Name=${NAME}' \
		-X 'webproxy/acc/libgo/biz.Build.Desc=${NAME} for ${GOOS}/${GOARCH}, v${VER}' \
		-v -s -w ${EXT_LDFLAGS}" -o ${OUTDIR}/${BIN} ${SRCDIR}/.

run: build
	cd ${OUTDIR} && ./${BIN} ${ARG}

surun: build
	cd ${OUTDIR} && sudo ./${BIN} ${ARG}

clean:
	GOOS=${GOOS} GOARCH=${GOARCH} go clean -tags="${TAGS}"  ${SRCDIR}/.

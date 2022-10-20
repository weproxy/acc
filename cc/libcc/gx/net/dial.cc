//
// weproxy@foxmail.com 2022/10/03
//

#include "gx/fmt/fmt.h"
#include "net.h"
#include "xx.h"

namespace gx {
namespace net {

////////////////////////////////////////////////////////////////////////////////
//
Dialer DefaultDialer;

namespace xx {
////////////////////////////////////////////////////////////////////////////////
// tcpConn_t for Dial() ...
struct tcpConn_t : public conn_t {
    tcpConn_t(SOCKET fd) : fd_(fd) {}
    virtual ~tcpConn_t() { Close(); }

    // Read ...
    virtual R<int, error> Read(slice<> b) override {
        if (fd_ <= 0) {
            return {0, ErrClosed};
        } else if (len(b) <= 0) {
            return {0, nil};
        }

        int r = co::recv(fd_, b.data(), len(b), timeoutMs(dealine_.d, dealine_.r));
        if (r <= 0) {
            if (co::error() == EAGAIN) {
                return {r, ErrClosed};
            }
            return {r, errors::New(co::strerror())};
        }
        return {r, nil};
    }

    // Write ...
    virtual R<int, error> Write(const slice<> b) override {
        if (fd_ <= 0) {
            return {0, ErrClosed};
        } else if (len(b) <= 0) {
            return {0, nil};
        }

        int r = co::send(fd_, b.data(), len(b), timeoutMs(dealine_.d, dealine_.w));
        if (r <= 0) {
            if (co::error() == EAGAIN) {
                return {r, ErrClosed};
            }
            return {r, errors::New(co::strerror())};
        }
        return {r, nil};
    }

    // Close ...
    virtual error Close() override {
        if (fd_ > 0) {
            co::close(fd_);
            fd_ = INVALID_SOCKET;
        }
        return nil;
    }

    // SetDeadline ...
    virtual error SetDeadline(const time::Time& t) override {
        dealine_.d = t;
        if (t && time::Since(t) >= 0) {
            Close();
        }
        return nil;
    }
    virtual error SetReadDeadline(const time::Time& t) override {
        dealine_.r = t;
        if (t && time::Since(t) >= 0) {
            CloseRead();
        }
        return nil;
    }
    virtual error SetWriteDeadline(const time::Time& t) override {
        dealine_.w = t;
        if (t && time::Since(t) >= 0) {
            CloseWrite();
        }
        return nil;
    }

    virtual int Fd() const override { return fd_; }
    virtual Addr LocalAddr() const override { return xx::GetSockAddr(fd_); }
    virtual Addr RemoteAddr() const override { return xx::GetPeerAddr(fd_); }
    virtual string String() const override { return GX_SS("tcpConn_t{" << RemoteAddr() << "}"); }

   public:
    SOCKET fd_{0};
    struct {
        time::Time d, r, w;
    } dealine_;
};
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////
// Dial ...
R<Conn, error> Dialer::Dial(const string& addr, int ms) {
    AUTO_R(ainfo, err, xx::GetAddrInfo(addr));
    if (err) {
        return {nil, err};
    }
    auto info = ainfo->i;

    SOCKET fd = co::tcp_socket(info->ai_family);
    if (fd < 0) {
        return {nil, fmt::Errorf("create socket error: %s", co::strerror())};
    }

    if (this->LocalAddr) {
        AUTO_R(ainfo, err, xx::GetAddrInfo(addr));
        if (err) {
            co::close(fd);
            return {nil, err};
        }
        auto info = ainfo->i;

        int r = co::bind(fd, info->ai_addr, (int)info->ai_addrlen);
        if (r < 0) {
            co::close(fd);
            return {nil, fmt::Errorf("bind error: %s", co::strerror())};
        }
    }

    if (this->Control) {
        this->Control(addr, fd);
    }

    if (ms <= 0 && this->Timeout) {
        ms = this->Timeout.Milliseconds();
    }

    int r = co::connect(fd, info->ai_addr, (int)info->ai_addrlen, ms);
    if (r != 0) {
        co::close(fd);
        return {nil, fmt::Errorf("dial %s error: %s", addr.c_str(), co::strerror())};
    }

    co::set_tcp_nodelay(fd);

    return {NewRef<xx::tcpConn_t>(fd), nil};
}

// Dial ...
R<Conn, error> Dialer::Dial(const Addr addr, int ms) {
    if (!addr) {
        return {nil, errors::New("addr is nil")};
    }
    return Dial(addr->String(), ms);
}

}  // namespace net
}  // namespace gx

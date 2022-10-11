//
// weproxy@foxmail.com 2022/10/03
//

#include "fmt/fmt.h"
#include "net.h"
#include "xx.h"

namespace gx {
namespace net {

////////////////////////////////////////////////////////////////////////////////////////////////////
//
Dialer DefaultDialer;

namespace xx {
////////////////////////////////////////////////////////////////////////////////////////////////////
// tcpCli_t ...
struct tcpCli_t : public conn_t {
    tcpCli_t(int fd) : fd_(fd) {}

    virtual ~tcpCli_t() { Close(); }

    virtual string String() { return "tcpCli_t"; }

    // Read ...
    virtual R<size_t, error> Read(void* b, size_t n) {
        int l = co::recv(fd_, b, n, timeoutMs(dealine_.d, dealine_.r));
        if (l <= 0) {
            if (co::error() == EAGAIN) {
                return {l, ErrClosed};
            }
            return {l, errors::New(co::strerror())};
        }
        return {l, nil};
    }

    // Write ...
    virtual R<size_t, error> Write(const void* b, size_t n) {
        int l = co::send(fd_, b, n, timeoutMs(dealine_.d, dealine_.w));
        if (l <= 0) {
            if (co::error() == EAGAIN) {
                return {l, ErrClosed};
            }
            return {l, errors::New(co::strerror())};
        }
        return {l, nil};
    }

    // Close ...
    virtual void Close() {
        if (fd_ > 0) {
            co::close(fd_);
            fd_ = 0;
        }
    }

    virtual int Fd() const { return fd_; }
    virtual Addr LocalAddr() { return xx::GetSockAddr(fd_); }
    virtual Addr RemoteAddr() { return xx::GetPeerAddr(fd_); }

    virtual error SetDeadline(const time::Time& t) {
        dealine_.d = t;
        return nil;
    }
    virtual error SetReadDeadline(const time::Time& t) {
        dealine_.r = t;
        return nil;
    }
    virtual error SetWriteDeadline(const time::Time& t) {
        dealine_.w = t;
        return nil;
    }

   public:
    int fd_{0};
    struct {
        time::Time d, r, w;
    } dealine_;
};
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// Dial ...
R<Conn, error> Dialer::Dial(const string& addr, int ms) {
    AUTO_R(ainfo, err, xx::GetAddrInfo(addr));
    if (err) {
        return {nil, err};
    }
    auto info = ainfo->i;

    sock_t fd = co::tcp_socket(info->ai_family);
    if (fd < 0) {
        return {nil, errors::New("create socket error: %s", co::strerror())};
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
            return {nil, errors::New("bind error: %s", co::strerror())};
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
        return {nil, errors::New("dial %s error: %s", addr.c_str(), co::strerror())};
    }

    co::set_tcp_nodelay(fd);

    std::shared_ptr<xx::tcpCli_t> c(new xx::tcpCli_t(fd));

    return {c, nil};
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

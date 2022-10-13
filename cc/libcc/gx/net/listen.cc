//
// weproxy@foxmail.com 2022/10/03
//

#include "co/str.h"
#include "co/tcp.h"
#include "net.h"
#include "xx.h"

namespace gx {
namespace net {

////////////////////////////////////////////////////////////////////////////////////////////////////
//
ListenConfig DefaultListenConfig;

namespace xx {
////////////////////////////////////////////////////////////////////////////////////////////////////
// tcpPeer_t ...
struct tcpPeer_t : public conn_t {
    tcpPeer_t(int fd) : c_(fd) {}
    virtual ~tcpPeer_t() { Close(); }

    // String ...
    virtual string String() override { return "tcpPeer_t"; }

    // Read ...
    virtual R<int, error> Read(byte_s b) override {
        int r = c_.recv(b.data(), b.size(), timeoutMs(dealine_.d, dealine_.r));
        if (r <= 0) {
            error err;
            if (co::error() == EAGAIN) {
                err = ErrClosed;
            } else {
                err = errors::New(c_.strerror());
            }
            return {r, err};
        }
        return {r, nil};
    }

    // Write ...
    virtual R<int, error> Write(const byte_s b) override {
        int r = c_.send(b.data(), b.size(), timeoutMs(dealine_.d, dealine_.w));
        if (r <= 0) {
            error err;
            if (co::error() == EAGAIN) {
                err = ErrClosed;
            } else {
                err = errors::New(c_.strerror());
            }
            return {r, err};
        }
        return {r, nil};
    }

    // Close ...
    virtual void Close() override { c_.close(); }

    virtual int Fd() const override { return c_.socket(); }
    virtual Addr LocalAddr() override { return GetSockAddr(c_.socket()); }
    virtual Addr RemoteAddr() override { return GetPeerAddr(c_.socket()); }

    virtual error SetDeadline(const time::Time& t) override {
        dealine_.d = t;
        return nil;
    }
    virtual error SetReadDeadline(const time::Time& t) override {
        dealine_.r = t;
        return nil;
    }
    virtual error SetWriteDeadline(const time::Time& t) override {
        dealine_.w = t;
        return nil;
    }

   private:
    ::tcp::Connection c_;
    struct {
        time::Time d, r, w;
    } dealine_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// tcpServ_t ...
struct tcpServ_t : public listener_t {
    tcpServ_t(sock_t fd) : fd_(fd) {}
    virtual ~tcpServ_t() { Close(); }

    // Accept ...
    virtual R<Conn, error> Accept() {
        addr_in_t addr;
        int addrlen = sizeof(addr);

        sock_t fd = co::accept(fd_, &addr, &addrlen);
        if (fd < 0) {
            return {nil, errors::New(co::strerror())};
        }

        co::set_tcp_keepalive(fd);
        co::set_tcp_nodelay(fd);

        return {std::shared_ptr<tcpPeer_t>(new tcpPeer_t(fd)), nil};
    }

    // Close ...
    virtual void Close() {
        if (fd_ > 0) {
            co::close(fd_);
            fd_ = 0;
        }
    }

    virtual net::Addr Addr() { return xx::GetSockAddr(fd_); }
    virtual int Fd() const override { return fd_; }

   private:
    sock_t fd_{0};
};

// udpConn_t ...
struct udpConn_t : public packetConn_t {
    udpConn_t(sock_t fd) : fd_(fd) {}

    virtual int Fd() const { return fd_; }

    // ReadFrom ...
    virtual R<int, Addr, error> ReadFrom(byte_s b) {
        if (fd_ <= 0) {
            return {0, nil, ErrClosed};
        }

        addr_in_t addr;
        int addrlen = sizeof(addr);

        int r = co::recvfrom(fd_, b.data(), b.size(), &addr, &addrlen, timeoutMs(dealine_.d, dealine_.r));
        if (r <= 0) {
            error err;
            if (co::error() == EAGAIN) {
                err = ErrClosed;
            } else {
                err = errors::New(co::strerror());
            }
            return {r, nil, err};
        }

        return {r, ToAddr(addr), nil};
    }

    // WriteTo ...
    virtual R<int, error> WriteTo(const byte_s b, Addr raddr) {
        if (fd_ <= 0) {
            return {0, ErrClosed};
        }

        AUTO_R(addr, addrlen, FromAddr(raddr));

        int r = co::sendto(fd_, b.data(), b.size(), &addr, addrlen, timeoutMs(dealine_.d, dealine_.w));
        if (r <= 0) {
            error err;
            if (co::error() == EAGAIN) {
                err = ErrClosed;
            } else {
                err = errors::New(co::strerror());
            }
            return {r, err};
        }

        return {r, nil};
    }

    virtual void Close() {
        if (fd_ > 0) {
            co::close(fd_);
            fd_ = 0;
        }
    };

    virtual Addr LocalAddr() { return GetSockAddr(fd_); };

    virtual error SetDeadline(const time::Time& t) override {
        dealine_.d = t;
        return nil;
    }
    virtual error SetReadDeadline(const time::Time& t) override {
        dealine_.r = t;
        return nil;
    }
    virtual error SetWriteDeadline(const time::Time& t) override {
        dealine_.w = t;
        return nil;
    }

    virtual string String() { return "udpConn_t"; }

   private:
    sock_t fd_{0};
    struct {
        time::Time d, r, w;
    } dealine_;
};

}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
// Listen ...
R<Listener, error> ListenConfig::Listen(const string& addr) {
    AUTO_R(ainfo, err, xx::GetAddrInfo(addr));
    if (err) {
        return {nil, err};
    }
    auto info = ainfo->i;

    sock_t fd = co::tcp_socket(info->ai_family);
    if (fd < 0) {
        return {nil, errors::New("create socket error: %s", co::strerror())};
    }

    co::set_reuseaddr(fd);

    // turn off IPV6_V6ONLY
    if (info->ai_family == AF_INET6) {
        int on = 0;
        co::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
    }

    int r = co::bind(fd, info->ai_addr, (int)info->ai_addrlen);
    if (r < 0) {
        co::close(fd);
        return {nil, errors::New("bind %s failed: %s", addr.c_str(), co::strerror())};
    }

    r = co::listen(fd, 64 * 1024);
    if (r < 0) {
        return {nil, errors::New("listen error: %s", co::strerror())};
    }

    std::shared_ptr<xx::tcpServ_t> ln(new xx::tcpServ_t(fd));

    return {ln, nil};
}

// Listen ...
R<Listener, error> ListenConfig::Listen(const Addr addr) {
    if (!addr) {
        return {nil, errors::New("addr is nil")};
    }
    return Listen(addr->String());
}

// ListenPacket ...
R<PacketConn, error> ListenConfig::ListenPacket(const string& addr) {
    AUTO_R(ainfo, err, xx::GetAddrInfo(addr));
    if (err) {
        return {nil, err};
    }
    auto info = ainfo->i;

    sock_t fd = co::udp_socket(info->ai_family);
    if (fd < 0) {
        return {nil, errors::New("create socket error: %s", co::strerror())};
    }

    co::set_reuseaddr(fd);

    // turn off IPV6_V6ONLY
    if (info->ai_family == AF_INET6) {
        int on = 0;
        co::setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
    }

    int r = co::bind(fd, info->ai_addr, (int)info->ai_addrlen);
    if (r < 0) {
        co::close(fd);
        return {nil, errors::New("bind %s failed: %s", addr.c_str(), co::strerror())};
    }

    std::shared_ptr<xx::udpConn_t> pc(new xx::udpConn_t(fd));

    return {pc, nil};
}

}  // namespace net
}  // namespace gx

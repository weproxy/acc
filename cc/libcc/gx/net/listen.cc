//
// weproxy@foxmail.com 2022/10/03
//

#include "co/str.h"
#include "net.h"
#include "xx.h"

namespace gx {
namespace net {

////////////////////////////////////////////////////////////////////////////////////////////////////
//
ListenConfig DefaultListenConfig;

namespace xx {
////////////////////////////////////////////////////////////////////////////////////////////////////
// tcpPeer_t for Accept() ...
struct tcpPeer_t : public conn_t {
    tcpPeer_t(SOCKET fd) : fd_(fd) {}
    virtual ~tcpPeer_t() { Close(); }

    // String ...
    virtual string String() const override {
        // return "";
        return GX_SS("tcpPeer_t{" << RemoteAddr() << "}");
    }

    // Read ...
    virtual R<int, error> Read(slice<byte> b) override {
        if (len(b) <= 0) {
            return {0, io::ErrShortBuffer};
        }
        if (fd_ <= 0) {
            return {0, ErrClosed};
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
    virtual R<int, error> Write(const slice<byte> b) override {
        if (len(b) <= 0) {
            return {0, io::ErrShortBuffer};
        }
        if (fd_ <= 0) {
            return {0, ErrClosed};
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

    virtual SOCKET Fd() const override { return fd_; }

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

   private:
    SOCKET fd_{INVALID_SOCKET};
    struct {
        time::Time d, r, w;
    } dealine_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// tcpServ_t ...
struct tcpServ_t : public listener_t {
    tcpServ_t(SOCKET fd) : fd_(fd) {}
    virtual ~tcpServ_t() { Close(); }

    // Accept ...
    virtual R<Conn, error> Accept() override {
        if (fd_ <= 0) {
            return {nil, ErrClosed};
        }

        addr_in_t addr;
        int addrlen = sizeof(addr);

        SOCKET fd = co::accept(fd_, &addr, &addrlen);
        if (fd <= 0) {
            return {nil, errors::New(co::strerror())};
        }

        co::set_tcp_keepalive(fd);
        co::set_tcp_nodelay(fd);

        return {MakeRef<tcpPeer_t>(fd), nil};
    }

    // Close ...
    virtual error Close() override {
        if (fd_ > 0) {
            co::close(fd_);
            fd_ = INVALID_SOCKET;
        }
        return nil;
    }

    virtual SOCKET Fd() const override { return fd_; }

   private:
    SOCKET fd_{INVALID_SOCKET};
};

// udpConn_t ...
struct udpConn_t : public packetConn_t {
    udpConn_t(SOCKET fd) : fd_(fd) {}
    virtual ~udpConn_t() { Close(); }

    // ReadFrom ...
    virtual R<int, Addr, error> ReadFrom(slice<byte> b) override {
        if (len(b) <= 0) {
            return {0, nil, io::ErrShortBuffer};
        }
        if (fd_ <= 0) {
            return {0, nil, ErrClosed};
        }

        addr_in_t addr;
        int addrlen = sizeof(addr);

        int r = co::recvfrom(fd_, b.data(), len(b), &addr, &addrlen, timeoutMs(dealine_.d, dealine_.r));
        if (r <= 0) {
            if (co::error() == EAGAIN) {
                return {r, nil, ErrClosed};
            }
            return {r, nil, errors::New(co::strerror())};
        }

        return {r, ToAddr(addr), nil};
    }

    // WriteTo ...
    virtual R<int, error> WriteTo(const slice<byte> b, Addr raddr) override {
        if (len(b) <= 0) {
            return {0, io::ErrShortBuffer};
        }
        if (fd_ <= 0) {
            return {0, ErrClosed};
        }

        AUTO_R(addr, addrlen, FromAddr(raddr));

        int r = co::sendto(fd_, b.data(), len(b), &addr, addrlen, timeoutMs(dealine_.d, dealine_.w));
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
    virtual string String() const override { return "udpConn_t"; }

   private:
    SOCKET fd_{INVALID_SOCKET};
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

    SOCKET fd = co::tcp_socket(info->ai_family);
    if (fd <= 0) {
        return {nil, fmt::Errorf("create socket error: %s", co::strerror())};
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
        return {nil, fmt::Errorf("bind %s failed: %s", addr.c_str(), co::strerror())};
    }

    r = co::listen(fd, 64 * 1024);
    if (r < 0) {
        return {nil, fmt::Errorf("listen error: %s", co::strerror())};
    }

    return {MakeRef<xx::tcpServ_t>(fd), nil};
}

// ListenPacket ...
R<PacketConn, error> ListenConfig::ListenPacket(const string& addr) {
    AUTO_R(ainfo, err, xx::GetAddrInfo(addr));
    if (err) {
        return {nil, err};
    }
    auto info = ainfo->i;

    SOCKET fd = co::udp_socket(info->ai_family);
    if (fd <= 0) {
        return {nil, fmt::Errorf("create socket error: %s", co::strerror())};
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
        return {nil, fmt::Errorf("bind %s failed: %s", addr.c_str(), co::strerror())};
    }

    return {MakeRef<xx::udpConn_t>(fd), nil};
}

}  // namespace net
}  // namespace gx

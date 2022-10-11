//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "builtin/builtin.h"
#include "errors/errors.h"
#include "fmt/fmt.h"
#include "io/io.h"
#include "ip.h"
#include "time/time.h"

namespace gx {
namespace net {

////////////////////////////////////////////////////////////////////////////////////////////////////
// error
extern const error ErrClosed;

////////////////////////////////////////////////////////////////////////////////////////////////////
// addr_t ...
namespace xx {
struct addr_t final {
    IP IP;
    uint16_t Port{0};

    string String() const;
};
}  // namespace xx

// Addr ...
typedef std::shared_ptr<xx::addr_t> Addr;

// MakeAddr ...
inline Addr MakeAddr(const IP& ip = IPv4zero, int port = 0) {
    Addr p(new xx::addr_t);
    p->IP = ip;
    p->Port = port;
    return p;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// conn_t ...
namespace xx {
struct conn_t : public io::ICloser {
    virtual ~conn_t() { Close(); }

    virtual int Fd() const = 0;

    virtual R<size_t, error> Read(void* b, size_t n) = 0;
    virtual R<size_t, error> Write(const void* b, size_t n) = 0;
    virtual void Close(){};

    virtual Addr LocalAddr() = 0;
    virtual Addr RemoteAddr() = 0;

    virtual error SetDeadline(const time::Time& t) { return nil; }
    virtual error SetReadDeadline(const time::Time& t) { return nil; }
    virtual error SetWriteDeadline(const time::Time& t) { return nil; }

    virtual string String() { return ""; }
};
}  // namespace xx

// Conn ...
typedef std::shared_ptr<xx::conn_t> Conn;

////////////////////////////////////////////////////////////////////////////////////////////////////
// packetConn_t ...
namespace xx {
struct packetConn_t : public io::ICloser {
    virtual ~packetConn_t() { Close(); }

    virtual int Fd() const = 0;

    virtual R<size_t, Addr, error> ReadFrom(void* b, size_t n) = 0;
    virtual R<size_t, error> WriteTo(const void* b, size_t n, Addr addr) = 0;
    virtual void Close(){};

    virtual Addr LocalAddr() = 0;

    virtual error SetDeadline(const time::Time& t) { return nil; }
    virtual error SetReadDeadline(const time::Time& t) { return nil; }
    virtual error SetWriteDeadline(const time::Time& t) { return nil; }

    virtual string String() { return ""; }
};
}  // namespace xx

// PacketConn ...
typedef std::shared_ptr<xx::packetConn_t> PacketConn;

////////////////////////////////////////////////////////////////////////////////////////////////////
// listener_t ...
namespace xx {
struct listener_t : public io::ICloser {
    virtual ~listener_t() { Close(); }

    virtual int Fd() const = 0;

    virtual R<Conn, error> Accept() = 0;
    virtual void Close(){};

    virtual Addr Addr() = 0;

    virtual string String() { return ""; }
};
}  // namespace xx

// Listener ...
typedef std::shared_ptr<xx::listener_t> Listener;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct Dialer final {
    Addr LocalAddr;
    time::Duration Timeout;
    std::function<error(const string& addr, int fd)> Control;

   public:
    // Dial ...
    R<Conn, error> Dial(const string& addr, int ms = 5000);
    R<Conn, error> Dial(const Addr addr, int ms = 5000);
};

// DefaultDialer ...
extern Dialer DefaultDialer;

// Dial ...
inline R<Conn, error> Dial(const string& addr, int ms = 5000) { return DefaultDialer.Dial(addr, ms); }
inline R<Conn, error> Dial(const Addr addr, int ms = 5000) { return DefaultDialer.Dial(addr, ms); }

////////////////////////////////////////////////////////////////////////////////////////////////////
//

// ListenConfig ...
struct ListenConfig final {
    std::function<error(const string& addr, int fd)> Control;

   public:
    // Listen ...
    R<Listener, error> Listen(const string& addr);
    R<Listener, error> Listen(const Addr addr);

    // ListenPacket ...
    R<PacketConn, error> ListenPacket(const string& addr);
    R<PacketConn, error> ListenPacket(const Addr addr);
};

// DefaultListenConfig ...
extern ListenConfig DefaultListenConfig;

// Listen ...
inline R<Listener, error> Listen(const string& addr) { return DefaultListenConfig.Listen(addr); }
inline R<Listener, error> Listen(const Addr addr) { return DefaultListenConfig.Listen(addr); }

// ListenPacket ...
inline R<PacketConn, error> ListenPacket(const string& addr) { return DefaultListenConfig.ListenPacket(addr); }
inline R<PacketConn, error> ListenPacket(const Addr addr) { return DefaultListenConfig.ListenPacket(addr); }

////////////////////////////////////////////////////////////////////////////////////////////////////
//

// SplitHostPort ...
R<string, int, error> SplitHostPort(const string& addr);

// JoinHostPort ...
string JoinHostPort(const string& host, const string& port);
string JoinHostPort(const string& host, int port);

// LookupHost ...
R<Vec<Addr>, error> LookupHost(const string& host);

// LookupIP ...
R<Vec<IP>, error> LookupIP(const string& host);

////////////////////////////////////////////////////////////////////////////////////////////////////
//

// Flags ...
enum Flags {
    FlagUp = 1 << 1,            // interface is up
    FlagBroadcast = 2 << 1,     // interface supports broadcast access capability
    FlagLoopback = 3 << 1,      // interface is a loopback interface
    FlagPointToPoint = 4 << 1,  // interface belongs to a point-to-point link
    FlagMulticast = 5 << 1,     // interface supports multicast access capability
};

// HardwareAddr ...
struct HardwareAddr {
    uint8 B[6];

    string String(bool upper = false) const;
};

// ParseMAC ...
R<HardwareAddr, error> ParseMAC(const string& s);

// IInterface
namespace xx {
struct interface_t {
    int Index{-1};
    int MTU{0};
    string Name;
    HardwareAddr HardwareAddr;
    Flags Flags;

    // Addrs ...
    R<Vec<Addr>, error> Addrs();
};
}  // namespace xx

// Interface ...
typedef std::shared_ptr<xx::interface_t> Interface;

// Interfaces ...
R<Vec<Interface>, error> Interfaces();

// InterfaceAddrs ...
R<Vec<Addr>, error> InterfaceAddrs();

// InterfaceByIndex ...
R<Interface, error> InterfaceByIndex(int index);

// InterfaceByName ...
R<Interface, error> InterfaceByName(const string& name);

}  // namespace net
}  // namespace gx

#include "xx.h"

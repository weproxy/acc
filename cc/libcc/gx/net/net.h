//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include "gx/builtin/builtin.h"
#include "gx/errors/errors.h"
#include "gx/fmt/fmt.h"
#include "gx/io/io.h"
#include "gx/time/time.h"
#include "ip.h"

namespace gx {
namespace net {

#ifndef _WIN32
typedef int SOCKET;
#define INVALID_SOCKET -1
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// error
extern const error ErrClosed;

////////////////////////////////////////////////////////////////////////////////////////////////////
// addr_t ...
namespace xx {
struct addr_t final {
    addr_t() = default;
    addr_t(const IP& ip, uint16 port) : IP(ip), Port(port) {}

    string String() const;

    IP IP;
    uint16 Port{0};
};

// Addr ...
typedef std::shared_ptr<addr_t> Addr;
}  // namespace xx

using xx::Addr;

// MakeAddr ...
inline Addr MakeAddr(const IP& ip = IPv4zero, int port = 0) { return Addr(new xx::addr_t(ip, port)); }

////////////////////////////////////////////////////////////////////////////////////////////////////
// conn_t ...
namespace xx {
struct conn_t : public io::ICloser {
    virtual ~conn_t() { Close(); }

    virtual R<int, error> Read(byte_s b) = 0;
    virtual R<int, error> Write(const byte_s b) = 0;

    virtual void Close(){};
    virtual void CloseRead() { Close(); }
    virtual void CloseWrite() { Close(); }

    virtual error SetDeadline(const time::Time& t) { return nil; }
    virtual error SetReadDeadline(const time::Time& t) { return nil; }
    virtual error SetWriteDeadline(const time::Time& t) { return nil; }

    virtual SOCKET Fd() const = 0;
    virtual Addr LocalAddr() = 0;
    virtual Addr RemoteAddr() = 0;
    virtual string String() { return ""; }
};

// Conn ...
typedef std::shared_ptr<conn_t> Conn;

struct connWrap_t : public conn_t {
    Conn wrap_;

    connWrap_t() = default;
    connWrap_t(Conn c) : wrap_(c) {}

    virtual R<int, error> Read(byte_s buf) { return wrap_->Read(buf); }
    virtual R<int, error> Write(const byte_s buf) { return wrap_->Write(buf); }

    virtual void Close() { wrap_->Close(); };
    virtual void CloseRead() { wrap_->CloseRead(); }
    virtual void CloseWrite() { wrap_->CloseWrite(); }

    virtual error SetDeadline(const time::Time& t) { return wrap_->SetDeadline(t); }
    virtual error SetReadDeadline(const time::Time& t) { return wrap_->SetReadDeadline(t); }
    virtual error SetWriteDeadline(const time::Time& t) { return wrap_->SetWriteDeadline(t); }

    virtual SOCKET Fd() const { return wrap_->Fd(); };
    virtual Addr LocalAddr() { return wrap_->LocalAddr(); };
    virtual Addr RemoteAddr() { return wrap_->RemoteAddr(); };
    virtual string String() { return wrap_->String(); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// packetConn_t ...
struct packetConn_t : public io::ICloser {
    virtual ~packetConn_t() { Close(); }

    virtual R<int, Addr, error> ReadFrom(byte_s b) = 0;
    virtual R<int, error> WriteTo(const byte_s b, Addr addr) = 0;

    virtual void Close(){};
    virtual void CloseRead() { Close(); }
    virtual void CloseWrite() { Close(); }

    virtual error SetDeadline(const time::Time& t) { return nil; }
    virtual error SetReadDeadline(const time::Time& t) { return nil; }
    virtual error SetWriteDeadline(const time::Time& t) { return nil; }

    virtual SOCKET Fd() const = 0;
    virtual Addr LocalAddr() = 0;
    virtual string String() { return ""; }
};

typedef std::shared_ptr<packetConn_t> PacketConn;

struct packetConnWrap_t : public packetConn_t {
    PacketConn wrap_;

    packetConnWrap_t() = default;
    packetConnWrap_t(PacketConn pc) : wrap_(pc) {}

    virtual R<int, net::Addr, error> ReadFrom(byte_s buf) { return wrap_->ReadFrom(buf); }
    virtual R<int, error> WriteTo(const byte_s buf, Addr addr) { return wrap_->WriteTo(buf, addr); }

    virtual void Close() { wrap_->Close(); };
    virtual void CloseRead() { wrap_->CloseRead(); }
    virtual void CloseWrite() { wrap_->CloseWrite(); }

    virtual error SetDeadline(const time::Time& t) { return wrap_->SetDeadline(t); }
    virtual error SetReadDeadline(const time::Time& t) { return wrap_->SetReadDeadline(t); }
    virtual error SetWriteDeadline(const time::Time& t) { return wrap_->SetWriteDeadline(t); }

    virtual SOCKET Fd() const { return wrap_->Fd(); };
    virtual Addr LocalAddr() { return wrap_->LocalAddr(); };
    virtual string String() { return wrap_->String(); }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// listener_t ...
struct listener_t : public io::ICloser {
    virtual ~listener_t() { Close(); }

    virtual R<Conn, error> Accept() = 0;

    virtual void Close(){};

    virtual SOCKET Fd() const = 0;
    virtual Addr Addr() = 0;
    virtual string String() { return ""; }
};

// Listener ...
typedef std::shared_ptr<listener_t> Listener;
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////
//
using xx::Conn;
using xx::Listener;
using xx::PacketConn;

////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct Dialer final {
    Addr LocalAddr;
    time::Duration Timeout;
    std::function<error(const string& addr, SOCKET fd)> Control;

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
    std::function<error(const string& addr, SOCKET fd)> Control;

   public:
    // Listen ...
    R<Listener, error> Listen(const string& addr);
    R<Listener, error> Listen(const Addr addr) { return Listen(addr ? addr->String() : ""); }

    // ListenPacket ...
    R<PacketConn, error> ListenPacket(const string& addr);
    R<PacketConn, error> ListenPacket(const Addr addr) { return ListenPacket(addr ? addr->String() : ""); }
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

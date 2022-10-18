//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <atomic>

#include "gx/time/time.h"
#include "gx/x/time/rate/rate.h"
#include "nx/nx.h"

namespace nx {
namespace stats {
using namespace gx;

constexpr const char* TAG = "[stats]";

enum Type {
    TypeDirect = 0,
    TypeS5,
    TypeSS,
    TypeGAAP,
    TypeHTP,
    TypeDNS,
    TypeKCP,
    TypeQUIC,
    TypeOTHER,
    typeMax,
};

// ToString ...
inline const char* ToString(Type typ) {
    static const char* S[] = {"Direct", "S5", "SS", "GAAP", "HTP", "DNS", "KCP", "QUIC", "OTH", ""};
    return S[typ - TypeDirect];
}

namespace xx {

constexpr int typeCount = typeMax - TypeDirect;

typedef std::atomic_bool abool;
typedef std::atomic<int32> aint32;
typedef std::atomic<int64> aint64;

// connE of entry ...
struct connE {
    aint64 tcp{0}, tcpTotal{0};
    aint64 udp{0}, udpTotal{0};

    string String() const;
};

// connT of type ...
struct connT {
    abool dirty{false};
    connE arr[typeCount];

    connT& AddTCP(Type typ, int delta = 1) {
        dirty = true;
        arr[typ].tcp += delta;
        if (delta > 0) {
            arr[typ].tcpTotal += delta;
        }
        return *this;
    }
    connT& AddUDP(Type typ, int delta = 1) {
        dirty = true;
        arr[typ].udp += delta;
        if (delta > 0) {
            arr[typ].udpTotal += delta;
        }
        return *this;
    }

    int64 GetTCP(Type typ) { return arr[typ].tcp; }
    int64 GetUDP(Type typ) { return arr[typ].udp; }
    int64 GetTCPTotal(Type typ) { return arr[typ].tcpTotal; }
    int64 GetUDPTotal(Type typ) { return arr[typ].udpTotal; }

    void calc();
};

// rateE of entry ...
struct rateE {
    aint64 sent{0}, sentTotal{0};
    aint64 recv{0}, recvTotal{0};

    void calc(Type typ, bool tcp);
};

// rateT of type ...
struct rateT {
    rateE tcp[typeCount];
    rateE udp[typeCount];

    rateT& AddTCPSent(Type typ, int delta) {
        auto& p = tcp[typ];
        p.sent += delta;
        p.sentTotal += delta;
        return *this;
    }
    rateT& AddTCPRecv(Type typ, int delta) {
        auto& p = tcp[typ];
        p.recv += delta;
        p.recvTotal += delta;
        return *this;
    }
    rateT& AddUDPSent(Type typ, int delta) {
        auto& p = udp[typ];
        p.sent += delta;
        p.sentTotal += delta;
        return *this;
    }
    rateT& AddUDPRecv(Type typ, int delta) {
        auto& p = udp[typ];
        p.recv += delta;
        p.recvTotal += delta;
        return *this;
    }

    void calc() {
        for (int i = 0; i < typeCount; i++) {
            Type typ = (Type)(i + TypeDirect);
            tcp[i].calc(typ, true);
            udp[i].calc(typ, false);
        }
    }
};

enum {
    _dnsFlagSucceeded = 0,
    _dnsFlagFailed,
    _dnsFlagCacheHit,
    _dnsFlagDropped,
    _dnsFlagFaked,
};

// dnsI ...
struct dnsI {
    int flg{0};
    int cnt{0};

    dnsI& setbit(int bit, bool b) {
        if (b) {
            flg |= 1 << bit;
        } else {
            flg &= ~(1 << bit);
        }
        return *this;
    }
    bool getbit(int bit) { return (flg & (1 << bit)) != 0; }
};

// dnsE of entry ...
struct dnsE {
    aint64 total{0};
    aint64 succeeded{0};
    aint64 failed{0};
    aint64 cahcehit{0};
    aint64 dropped{0};
    aint64 faked{0};
};

// dnsT of type ...
struct dnsT {
    dnsE all;
    dnsE distinct;
    Map<string, dnsI> names;
    abool dirty{false};

    dnsT& AddQuery(const string& name) {
        if (!name.empty()) {
            dirty = true;
            all.total += 1;

            auto it = names.find(name);
            if (it == names.end()) {
                distinct.total += 1;
                names[name] = dnsI{};
            } else {
                it->second.cnt += 1;
            }
        }
        return *this;
    }

    dnsT& add(aint64& all, aint64& distinct, int bit, const string& name) { return *this; }

    dnsT& AddSucceeded(const string& name) { return add(all.succeeded, distinct.succeeded, _dnsFlagSucceeded, name); }
    dnsT& AddFailed(const string& name) { return add(all.failed, distinct.failed, _dnsFlagFailed, name); }
    dnsT& AddCacheHit(const string& name) { return add(all.cahcehit, distinct.cahcehit, _dnsFlagCacheHit, name); }
    dnsT& AddDropped(const string& name) { return add(all.dropped, distinct.dropped, _dnsFlagDropped, name); }
    dnsT& AddFaked(const string& name) { return add(all.faked, distinct.faked, _dnsFlagFaked, name); }

    void calc();
};

extern connT _Conn;
extern rateT _Rate;
extern dnsT _DNS;

// stats_t ...
struct stats_t {
    aint64 sent{0}, recv{0};
    aint64 sentx{0}, recvx{0};
    bool tcp{false};
    time::Time start;
    string tag;
    Type typ;
    connT& conn;
    rateT& rate;
    string target;
    string server;

    stats_t(Type typ_, const string& tag_, bool tcp_) : tcp(tcp_), typ(typ_), tag(tag_), conn(_Conn), rate(_Rate) {}

    const string& Tag() const { return tag; }
    stats_t& SetTag(const string& s) {
        tag = s;
        return *this;
    }

    const string& Target() const { return target; }
    stats_t& SetTarget(const string& s) {
        target = s;
        return *this;
    }

    const string& Server() const { return server; }
    stats_t& SetServer(const string& s) {
        server = s;
        return *this;
    }

    stats_t& AddSent(int n) {
        if (tcp) {
            rate.AddTCPSent(typ, n);
        } else {
            rate.AddUDPSent(typ, n);
        }
        sent += n;
        return *this;
    }

    stats_t& AddRecv(int n) {
        if (tcp) {
            rate.AddTCPRecv(typ, n);
        } else {
            rate.AddUDPRecv(typ, n);
        }
        recv += n;
        return *this;
    }

    stats_t& AddSentX(int n) {
        if (tcp) {
            rate.AddTCPSent(typ, n);
        } else {
            rate.AddUDPSent(typ, n);
        }
        sentx += n;
        return *this;
    }

    stats_t& AddRecvX(int n) {
        if (tcp) {
            rate.AddTCPRecv(typ, n);
        } else {
            rate.AddUDPRecv(typ, n);
        }
        recvx += n;
        return *this;
    }

    time::Duration Elapsed() const { return time::Since(start); }

    const stats_t& LogD(const string& msg) const;
    const stats_t& LogI(const string& msg) const;
    const stats_t& LogW(const string& msg) const;
    const stats_t& LogE(const string& msg) const;

    stats_t& Start(const string& msg = {});
    stats_t& Done(const string& msg = {});
};

}  // namespace xx

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Stats ...
using Stats = Ref<xx::stats_t>;

// NewStats ...
inline Stats NewStats(Type typ, const string& tag, bool tcp) { return Stats(new xx::stats_t(typ, tag, tcp)); }

// NewTCPStats ...
inline Stats NewTCPStats(Type typ, const string& tag) { return NewStats(typ, tag, true); }

// NewUDPStats ...
inline Stats NewUDPStats(Type typ, const string& tag) { return NewStats(typ, tag, false); }

}  // namespace stats
}  // namespace nx

//
// weproxy@foxmail.com 2022/10/03
//

#include "stats.h"

#include "fx/fx.h"
#include "gx/fmt/fmt.h"
#include "logx/logx.h"

namespace nx {
namespace stats {
namespace xx {

////////////////////////////////////////////////////////////////////////////////
//

connT _Conn;
rateT _Rate;
dnsT _DNS;

////////////////////////////////////////////////////////////////////////////////
//
string connE::String() const {
    return fmt::Sprintf("T:%lld/%lld,U:%lld/%lld", tcp.load(), tcpTotal.load(), udp.load(), udpTotal.load());
}

////////////////////////////////////////////////////////////////////////////////
//
void rateE::calc(Type typ, bool tcp) {
    int64 sentRate = sent.exchange(0);
    int64 recvRate = recv.exchange(0);

    if (sentRate > 0 || recvRate > 0) {
        LOGS_P("[rate] " << ToString(typ) << (tcp ? " TCP" : " UDP") << ": sent=" << fx::FormatBytes(sentRate)
                         << "/s, recv=" << fx::FormatBytes(recvRate) << "/s");
    }
}
////////////////////////////////////////////////////////////////////////////////
//
void dnsT::calc() {
    if (!dirty.exchange(false)) {
        return;
    }

    LOGS_P(TAG << " DNS{dist/indist}: total=" << distinct.total << "/" << all.total << ", succ=" << distinct.succeeded
               << "/" << all.succeeded << ", fail=" << distinct.failed << "/" << all.failed
               << ", hit=" << distinct.cahcehit << "/" << all.cahcehit << ", drop=" << distinct.dropped << "/"
               << all.dropped << ", fake=" << distinct.faked << "/" << all.faked);
}

////////////////////////////////////////////////////////////////////////////////
//
void connT::calc() {
    if (!dirty.exchange(false)) {
        return;
    }

    std::ostringstream oss;
    oss << TAG << " Conn{cur/his}: ";

    for (int i = 0; i < xx::typeCount; i++) {
        Type typ = (Type)(TypeDirect + i);
        if (typ == typeMax || (arr[i].tcpTotal == 0 || arr[i].tcpTotal == 0)) {
            continue;
        }

        oss << ToString(typ) << "{" << arr[i].String() << "} ";
    }

    LOGS_P(oss.str());
}
}  // namespace xx

// LogD ...
const Stats& Stats::LogD(const string& msg) const {
    LOGS_D(tag << " " << msg << ", " << Elapsed());
    return *this;
}

// LogI ...
const Stats& Stats::LogI(const string& msg) const {
    LOGS_I(tag << " " << msg << ", " << Elapsed());
    return *this;
}

// LogW ...
const Stats& Stats::LogW(const string& msg) const {
    LOGS_W(tag << " " << msg << ", " << Elapsed());
    return *this;
}

// LogE ...
const Stats& Stats::LogE(const string& msg) const {
    LOGS_E(tag << " " << msg << ", " << Elapsed());
    return *this;
}

// Start ...
Stats& Stats::Start(const string& msg) {
    start = time::Now();

    if (!msg.empty()) {
        LOGS_I(tag << " " << msg);
    }

    if (tcp) {
        conn.AddTCP(typ, 1);
    } else {
        conn.AddUDP(typ, 1);
    }

    return *this;
}

// Done ...
Stats& Stats::Done(const string& msg) {
    if (tcp) {
        conn.AddTCP(typ, -1);
    } else {
        conn.AddUDP(typ, -1);
    }

    if (sentx > 0 || recvx > 0) {
        LOGS_I(tag << " " << msg << ", sent=" << fx::FormatBytes(sentx) << "+" << fx::FormatBytes(sent) + ", recv="
                   << fx::FormatBytes(recvx) << "+" << fx::FormatBytes(recv) << ", " << Elapsed());
    } else {
        LOGS_I(tag << " " << msg << ", sent=" << fx::FormatBytes(sent) + ", recv=" << fx::FormatBytes(recv) << ", "
                   << Elapsed());
    }

    return *this;
}

////////////////////////////////////////////////////////////////////////////////
// init ...
namespace xx {
static auto _ = [] {
    gx::go([] {
        LOGS_V(TAG << " init()");
        for (;;) {
            _Rate.calc();
            _DNS.calc();
            _Conn.calc();
            time::Sleep(time::Second);
        }
    });
    return 0;
}();
}  // namespace xx
}  // namespace stats
}  // namespace nx

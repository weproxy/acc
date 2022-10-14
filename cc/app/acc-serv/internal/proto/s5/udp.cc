//
// weproxy@foxmail.com 2022/10/03
//

#include "fx/fx.h"
#include "gx/net/net.h"
#include "gx/time/time.h"
#include "logx/logx.h"
#include "nx/netio/netio.h"
#include "nx/socks/socks.h"
#include "nx/stats/stats.h"
#include "s5.h"

NAMESPACE_BEG_S5

namespace xx {
using namespace nx;

// udpConn_t to target server
struct udpConn_t : public net::xx::packetConnWrap_t {
    socks::AddrType typ_{socks::AddrTypeIPv4};

    udpConn_t(net::PacketConn pc) : net::xx::packetConnWrap_t(pc) {}

    // ReadFrom ...
    // readFrom target server, and pack data to client
    //
    // <<< RSP:
    //     | RSV | FRAG | ATYP | SRC.ADDR | SRC.PORT | DATA |
    //     +-----+------+------+----------+----------+------+
    //     |  2  |  1   |  1   |    ...   |    2     |  ... |
    virtual R<int, net::Addr, error> ReadFrom(byte_s buf) {
        if (socks::AddrTypeIPv4 != typ_ && socks::AddrTypeIPv6 != typ_) {
            return {0, nil, socks::ErrInvalidAddrType};
        }

        int pos = 3 + 1 + (socks::AddrTypeIPv4 == typ_ ? 4 : 16) + 2;
        AUTO_R(n, addr, err, wrap_->ReadFrom(buf(pos)));
        if (err) {
            return {n, addr, err};
        }

        auto raddr = socks::FromNetAddr(addr);
        socks::CopyAddr(buf(3), raddr);

        n += 3 + raddr->B.size();

        buf[0] = 0; // RSV
        buf[1] = 0; //
        buf[2] = 0; // FRAG

        return {n, addr, nil};
    }

    // WriteTo ...
    // unpack data from client, and writeTo target server
    //
    // >>> REQ:
    //     | RSV | FRAG | ATYP | DST.ADDR | DST.PORT | DATA |
    //     +-----+------+------+----------+----------+------+
    //     |  2  |  1   |  1   |    ...   |    2     |  ... |
    virtual R<int, error> WriteTo(byte_s buf) {
        if (buf.size() < 10) {
            return {0, socks::ErrInvalidSocksVersion};
        }

        AUTO_R(raddr, er1, socks::ParseAddr(buf(3)));
        if (er1) {
            return {0, er1};
        }

        typ_ = socks::AddrType(raddr->B[0]);
        if (socks::AddrTypeIPv4 != typ_ && socks::AddrTypeIPv6 != typ_) {
            return {0, socks::ErrInvalidAddrType};
        }

        int pos = 3 + raddr->B.size();
        AUTO_R(n, er2, wrap_->WriteTo(buf(pos), raddr->ToNetAddr()));
        if (er2) {
            return {n, er2};
        }

        return {n + pos, nil};
    }

    // wrap ...
    static net::PacketConn wrap(net::PacketConn pc) {
        //
        return std::shared_ptr<udpConn_t>(new udpConn_t(pc));
    }
};

// handleUDP ...
error handleUDP(net::PacketConn c, net::Addr caddr, net::Addr raddr) {
    auto tag = GX_SS(TAG << " UDP_" << nx::NewID() << " " << caddr << "->" << raddr);
    auto sta = stats::NewUDPStats(stats::TypeS5, tag);

    sta->Start("started");
    DEFER(sta->Done("closed"));

    AUTO_R(ln, err, net::ListenPacket(":0"));
    if (err) {
        return err;
    }

    auto rc = udpConn_t::wrap(ln);

    netio::RelayOption opt;
    opt.Read.CopingFn = [sta = sta](int n) { sta->AddRecv(n); };
    opt.Write.CopingFn = [sta = sta](int n) { sta->AddSent(n); };

    AUTO_R(read, written, er2, netio::Relay(c, rc, opt));
    if (er2) {
        if (er2 != net::ErrClosed) {
            LOGS_E(TAG << " relay " << tag << " , err: " << er2);
        }
        return er2;
    }

    return nil;
}

}  // namespace xx

NAMESPACE_END_S5

//
// weproxy@foxmail.com 2022/10/23
//

#include "dnsmessage.h"

namespace gx {
namespace dnsmessage {

// const int maxUint16 = ~uint16(0);
const int maxUint16 = 65535;

// packStartingCap is the default initial buffer size allocated during
// packing.
//
// The starting capacity doesn't matter too much, but most DNS responses
// Will be <= 512 bytes as it is the limit for DNS over UDP.
static const int packStartingCap = 512;

// uint16Len is the length (in bytes) of a uint16.
static const int uint16Len = 2;

// uint32Len is the length (in bytes) of a uint32.
static const int uint32Len = 4;

// headerLen is the length (in bytes) of a DNS header.
//
// A header is comprised of 6 uint16s and no padding.
static const int headerLen = 6 * uint16Len;

static const int headerBitQR = 1 << 15;  // query/response (response=1)
static const int headerBitAA = 1 << 10;  // authoritative
static const int headerBitTC = 1 << 9;   // truncated
static const int headerBitRD = 1 << 8;   // recursion desired
static const int headerBitRA = 1 << 7;   // recursion available
static const int headerBitAD = 1 << 5;   // authentic data
static const int headerBitCD = 1 << 4;   // checking disabled

// packUint16 appends the wire format of field to msg.
static bytez<> packUint16(bytez<> msg, uint16 field) { return append(msg, byte(field >> 8), byte(field)); }

// packType appends the wire format of field to msg.
static bytez<> packType(bytez<> msg, Type field) { return packUint16(msg, uint16(field)); }

// packClass appends the wire format of field to msg.
static bytez<> packClass(bytez<> msg, Class field) { return packUint16(msg, uint16(field)); }

// packUint32 appends the wire format of field to msg.
static bytez<> packUint32(bytez<> msg, uint32 field) {
    return append(msg, byte(field >> 24), byte(field >> 16), byte(field >> 8), byte(field));
}

// packText appends the wire format of field to msg.
static R<bytez<>, error> packText(bytez<> msg, const string& field) {
    int l = len(field);
    if (l > 255) {
        return {{}, xx::errStringTooLong};
    }
    msg = append(msg, byte(l));
    msg = append(msg, field);

    return {msg, nil};
}

// packBytes appends the wire format of field to msg.
static bytez<> packBytes(bytez<> msg, bytez<> field) { return append(msg, field); }

////////////////////////////////////////////////////////////////////////////////

// pack ...
R<uint16, uint16> Header::pack() {
    auto& m = *this;
    uint16 id = m.ID;
    uint16 bits = uint16(m.OpCode) << 11 | uint16(m.RCode);
    if (m.RecursionAvailable) {
        bits |= headerBitRA;
    }
    if (m.RecursionDesired) {
        bits |= headerBitRD;
    }
    if (m.Truncated) {
        bits |= headerBitTC;
    }
    if (m.Authoritative) {
        bits |= headerBitAA;
    }
    if (m.Response) {
        bits |= headerBitQR;
    }
    if (m.AuthenticData) {
        bits |= headerBitAD;
    }
    if (m.CheckingDisabled) {
        bits |= headerBitCD;
    }
    return {id, bits};
}

namespace xx {
// pack appends the wire format of the header to msg.
bytez<> header::pack(bytez<> msg) {
    auto& h = *this;
    msg = packUint16(msg, h.id);
    msg = packUint16(msg, h.bits);
    msg = packUint16(msg, h.questions);
    msg = packUint16(msg, h.answers);
    msg = packUint16(msg, h.authorities);
    return packUint16(msg, h.additionals);
}

// Header ...
dnsmessage::Header header::Header() {
    auto& h = *this;
    dnsmessage::Header r;
    r.ID = h.id;
    r.Response = (h.bits & headerBitQR) != 0;
    r.OpCode = OpCode(h.bits >> 11) & 0xF;
    r.Authoritative = (h.bits & headerBitAA) != 0;
    r.Truncated = (h.bits & headerBitTC) != 0;
    r.RecursionDesired = (h.bits & headerBitRD) != 0;
    r.RecursionAvailable = (h.bits & headerBitRA) != 0;
    r.AuthenticData = (h.bits & headerBitAD) != 0;
    r.CheckingDisabled = (h.bits & headerBitCD) != 0;
    r.RCode = RCode(h.bits & 0xF);
    return r;
}
}  // namespace xx

// pack appends the wire format of the Name to msg.
//
// Domain names are a sequence of counted strings split at the dots. They end
// with a zero-length string. Compression can be used to reuse domain suffixes.
//
// The compression map will be updated with new domain suffixes. If compression
// is nil, compression will not be used.
xx::packResult Name::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& n = *this;
    auto oldMsg = msg;

    // Add a trailing dot to canonicalize name.
    if (n.Length == 0 || n.Data[n.Length - 1] != '.') {
        return {oldMsg, xx::errNonCanonicalName};
    }

    // Allow root domain.
    if (n.Data[0] == '.' && n.Length == 1) {
        return {append(msg, 0), nil};
    }

    // Emit sequence of counted strings, chopping at dots.
    for (int i = 0, begin = 0; i < int(n.Length); i++) {
        // Check for the end of the segment.
        if (n.Data[i] == '.') {
            // The two most significant bits have special meaning.
            // It isn't allowed for segments to be long enough to
            // need them.
            if (i - begin >= 1 << 6) {
                return {oldMsg, xx::errSegTooLong};
            }

            // Segments must have a non-zero length.
            if (i - begin == 0) {
                return {oldMsg, xx::errZeroSegLen};
            }

            msg = append(msg, byte(i - begin));

            for (int j = begin; j < i; j++) {
                msg = append(msg, n.Data[j]);
            }

            begin = i + 1;
            continue;
        }

        // We can only compress domain suffixes starting with a new
        // segment. A pointer is two bytes with the two most significant
        // bits set to 1 to indicate that it is a pointer.
        if ((i == 0 || n.Data[i - 1] == '.') && compression.size() > 0) {
            AUTO_R(ptr, ok, compression(string(n.Data(i))));
            if (ok) {
                // Hit. Emit a pointer instead of the rest of
                // the domain.
                return {append(msg, byte((ptr >> 8) | 0xC0), byte(ptr)), nil};
            }

            // Miss. Add the suffix to the compression table if the
            // offset can be stored in the available 14 bytes.
            if (len(msg) <= int(uint16(0xffff) >> 2)) {
                auto b = n.Data(i);
                auto s = string((char*)b.data(), len(b));
                compression[s] = len(msg) - compressionOff;
            }
        }
    }
    return {append(msg, 0), nil};
}

R<bytez<> /*msg*/, int /*lenOff*/, error /*err*/> ResourceHeader::pack(bytez<> oldMsg, map<string, int> compression,
                                                                       int compressionOff) {
    auto& h = *this;
    AUTO_R(msg, err, h.Name.pack(oldMsg, compression, compressionOff));
    if (err != nil) {
        return {oldMsg, 0, xx::nestedError("Name", err)};
    }
    msg = packType(msg, h.Type);
    msg = packClass(msg, h.Class);
    msg = packUint32(msg, h.TTL);
    int lenOff = len(msg);
    msg = packUint16(msg, h.Length);
    return {msg, lenOff, nil};
}

error ResourceHeader::fixLen(bytez<> msg, int lenOff, int preLen) {
    auto& h = *this;
    int conLen = len(msg) - preLen;
    if (conLen > maxUint16) {
        return xx::errResTooLong;
    }

    // Fill in the length now that we know how long the content is.
    packUint16(msg(lenOff, lenOff), uint16(conLen));
    h.Length = uint16(conLen);

    return nil;
}

xx::packResult MXResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    auto oldMsg = msg;
    msg = packUint16(msg, r.Pref);
    AUTO_R(_msg, err, r.MX.pack(msg, compression, compressionOff));
    if (err != nil) {
        return {oldMsg, xx::nestedError("MXResource.MX", err)};
    }
    return {_msg, nil};
}

xx::packResult SOAResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    auto oldMsg = msg;
    AUTO_R(_msg1, _err1, r.NS.pack(msg, compression, compressionOff));
    if (_err1 != nil) {
        return {oldMsg, xx::nestedError("SOAResource.NS", _err1)};
    }
    AUTO_R(_msg2, _err2, r.MBox.pack(_msg1, compression, compressionOff));
    if (_err2 != nil) {
        return {oldMsg, xx::nestedError("SOAResource.MBox", _err2)};
    }
    msg = packUint32(_msg2, r.Serial);
    msg = packUint32(msg, r.Refresh);
    msg = packUint32(msg, r.Retry);
    msg = packUint32(msg, r.Expire);
    return {packUint32(msg, r.MinTTL), nil};
}

xx::packResult TXTResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    auto oldMsg = msg;
    for (int i = 0; i < len(r.TXT); i++) {
        AUTO_R(_msg, err, packText(msg, r.TXT[i]));
        if (err != nil) {
            return {oldMsg, err};
        }
        msg = _msg;
    }
    return {msg, nil};
}

xx::packResult SRVResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    auto oldMsg = msg;
    msg = packUint16(msg, r.Priority);
    msg = packUint16(msg, r.Weight);
    msg = packUint16(msg, r.Port);
    AUTO_R(_msg, err, r.Target.pack(msg, {}, compressionOff));
    if (err != nil) {
        return {oldMsg, xx::nestedError("SRVResource.Target", err)};
    }
    return {_msg, nil};
}

xx::packResult AResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    return {packBytes(msg, r.A(0, -1)), nil};
}

xx::packResult AAAAResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    return {packBytes(msg, r.AAAA(0, -1)), nil};
}

xx::packResult OPTResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    for (int i = 0; i < len(r.Options); ++i) {
        auto& opt = r.Options[i];
        msg = packUint16(msg, opt.Code);
        uint16 l = uint16(len(opt.Data));
        msg = packUint16(msg, l);
        msg = packBytes(msg, opt.Data);
    }
    return {msg, nil};
}

xx::packResult UnknownResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    return {packBytes(msg, r.Data(0, -1)), nil};
}

// pack appends the wire format of the Question to msg.
xx::packResult Question::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& q = *this;
    AUTO_R(_msg, err, q.Name.pack(msg, compression, compressionOff));
    msg = _msg;
    if (err != nil) {
        return {msg, xx::nestedError("Name", err)};
    }
    msg = packType(msg, q.Type);
    return {packClass(msg, q.Class), nil};
}

// pack appends the wire format of the Question to msg.
xx::packResult Resource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    if (r.Body == nil) {
        return {msg, xx::errNilResouceBody};
    }

    auto oldMsg = msg;
    r.Header.Type = r.Body->realType();

    AUTO_R(_msg1, lenOff, err1, r.Header.pack(msg, compression, compressionOff));
    msg = _msg1;
    if (err1 != nil) {
        return {msg, xx::nestedError("ResourceHeader", err1)};
    }

    int preLen = len(msg);
    AUTO_R(_msg2, err2, r.Body->pack(msg, compression, compressionOff));
    msg = _msg2;
    if (err2 != nil) {
        return {msg, xx::nestedError("content", err2)};
    }

    auto err = r.Header.fixLen(msg, lenOff, preLen);
    if (err != nil) {
        return {oldMsg, err};
    }

    return {msg, nil};
}

// Pack packs a full Message.
R<bytez<>, error> Message::Pack() { return AppendPack(make(0, packStartingCap)); }

// AppendPack is like Pack but appends the full Message to b and returns the
// extended buffer.
R<bytez<>, error> Message::AppendPack(bytez<> b) {
    auto& m = *this;
    // Validate the lengths. It is very unlikely that anyone will try to
    // pack more than 65535 of any particular type, but it is possible and
    // we should fail gracefully.
    if (len(m.Questions) > maxUint16) {
        return {{}, xx::errTooManyQuestions};
    }
    if (len(m.Answers) > maxUint16) {
        return {{}, xx::errTooManyAnswers};
    }
    if (len(m.Authorities) > maxUint16) {
        return {{}, xx::errTooManyAuthorities};
    }
    if (len(m.Additionals) > maxUint16) {
        return {{}, xx::errTooManyAdditionals};
    }

    xx::header h;
    AUTO_R(id, bits, m.Header.pack());
    h.id = id;
    h.bits = bits;

    h.questions = uint16(len(m.Questions));
    h.answers = uint16(len(m.Answers));
    h.authorities = uint16(len(m.Authorities));
    h.additionals = uint16(len(m.Additionals));

    int compressionOff = len(b);
    auto msg = h.pack(b);

    // RFC 1035 allows (but does not require) compression for packing. RFC
    // 1035 requires unpacking implementations to support compression, so
    // unconditionally enabling it is fine.
    //
    // DNS lookups are typically done over UDP, and RFC 1035 states that UDP
    // DNS messages can be a maximum of 512 bytes long. Without compression,
    // many DNS response messages are over this limit, so enabling
    // compression will help ensure compliance.
    map<string, int> compression;

    for (int i = 0; i < len(m.Questions); i++) {
        AUTO_R(_msg, _err, m.Questions[i].pack(msg, compression, compressionOff));
        if (_err != nil) {
            return {{}, xx::nestedError("packing Question", _err)};
        }
        msg = _msg;
    }
    for (int i = 0; i < len(m.Answers); i++) {
        AUTO_R(_msg, _err, m.Answers[i].pack(msg, compression, compressionOff));
        if (_err != nil) {
            return {{}, xx::nestedError("packing Answer", _err)};
        }
        msg = _msg;
    }
    for (int i = 0; i < len(m.Authorities); i++) {
        AUTO_R(_msg, _err, m.Authorities[i].pack(msg, compression, compressionOff));
        if (_err != nil) {
            return {{}, xx::nestedError("packing Authority", _err)};
        }
        msg = _msg;
    }
    for (int i = 0; i < len(m.Additionals); i++) {
        AUTO_R(_msg, _err, m.Additionals[i].pack(msg, compression, compressionOff));
        if (_err != nil) {
            return {{}, xx::nestedError("packing Additional", _err)};
        }
        msg = _msg;
    }

    return {msg, nil};
}

// NewBuilder creates a new builder with compression disabled.
//
// Note: Most users will want to immediately enable compression with the
// EnableCompression method. See that method's comment for why you may or may
// not want to enable compression.
//
// The DNS message is appended to the provided initial buffer buf (which may be
// nil) as it is built. The final message is returned by the (*Builder).Finish
// method, which includes buf[:len(buf)] and may return the same underlying
// array if there was sufficient capacity in the slice.
Builder NewBuilder(bytez<> buf, Header h) {
    if (!buf) {
        buf = make(0, packStartingCap);
    }
    Builder b;
    b.msg = buf;
    b.start = len(buf);
    AUTO_R(id, bits, h.pack());
    b.header.id = id;
    b.header.bits = bits;
    bytez<> hb(headerLen);
    b.msg = append(b.msg, hb);
    b.section = xx::sectionHeader;
    return b;
}

error Builder::incrementSectionCount() {
    auto& b = *this;
    uint16* count = 0;
    error err;
    switch (b.section) {
        case xx::sectionQuestions:
            count = &b.header.questions;
            err = xx::errTooManyQuestions;
            break;
        case xx::sectionAnswers:
            count = &b.header.answers;
            err = xx::errTooManyAnswers;
            break;
        case xx::sectionAuthorities:
            count = &b.header.authorities;
            err = xx::errTooManyAuthorities;
            break;
        case xx::sectionAdditionals:
            count = &b.header.additionals;
            err = xx::errTooManyAdditionals;
            break;
    }
    if (*count == uint16(maxUint16)) {
        return err;
    }
    (*count)++;
    return nil;
}

// Question adds a single Question.
error Builder::Question(dnsmessage::Question q) {
    auto& b = *this;
    if (b.section < xx::sectionQuestions) {
        return ErrNotStarted;
    }
    if (b.section > xx::sectionQuestions) {
        return ErrSectionDone;
    }
    AUTO_R(msg, err, q.pack(b.msg, b.compression, b.start));
    if (err != nil) {
        return err;
    }
    err = b.incrementSectionCount();
    if (err != nil) {
        return err;
    }
    b.msg = msg;
    return nil;
}

// addResourceBody adds a single ResourceBody.
error Builder::addResourceBody(ResourceHeader& h, dnsmessage::ResourceBody* r, const char* name) {
    auto& b = *this;
    auto err = b.checkResourceSection();
    if (err != nil) {
        return err;
    }
    h.Type = r->realType();
    AUTO_R(msg2, lenOff, err2, h.pack(b.msg, b.compression, b.start));
    if (err2 != nil) {
        return xx::nestedError("ResourceHeader", err2);
    }
    int preLen = len(msg);
    AUTO_R(msg3, err3, r->pack(msg2, b.compression, b.start));
    if (err3 != nil) {
        return xx::nestedError(name, err3);
    }
    err = h.fixLen(msg3, lenOff, preLen);
    if (err != nil) {
        return err;
    }
    err = b.incrementSectionCount();
    if (err != nil) {
        return err;
    }
    b.msg = msg3;
    return nil;
}

}  // namespace dnsmessage
}  // namespace gx

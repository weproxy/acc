//
// weproxy@foxmail.com 2022/10/23
//

#include "dnsmessage.h"

namespace gx {
namespace dnsmessage {

// ErrNotStarted indicates that the prerequisite information isn't
// available yet because the previous records haven't been appropriately
// parsed, skipped or finished.
error ErrNotStarted = errors::New("parsing/packing of this type isn't available yet");

// ErrSectionDone indicated that all records in the section have been
// parsed or finished.
error ErrSectionDone = errors::New("parsing/packing of this section has completed");

error errBaseLen = errors::New("insufficient data for base length type");
error errCalcLen = errors::New("insufficient data for calculated length type");
error errReserved = errors::New("segment prefix is reserved");
error errTooManyPtr = errors::New("too many pointers (>10)");
error errInvalidPtr = errors::New("invalid pointer");
error errNilResouceBody = errors::New("nil resource body");
error errResourceLen = errors::New("insufficient data for resource body length");
error errSegTooLong = errors::New("segment length too long");
error errZeroSegLen = errors::New("zero length segment");
error errResTooLong = errors::New("resource length too long");
error errTooManyQuestions = errors::New("too many Questions to pack (>65535)");
error errTooManyAnswers = errors::New("too many Answers to pack (>65535)");
error errTooManyAuthorities = errors::New("too many Authorities to pack (>65535)");
error errTooManyAdditionals = errors::New("too many Additionals to pack (>65535)");
error errNonCanonicalName = errors::New("name is not in canonical format (it must end with a .)");
error errStringTooLong = errors::New("character string exceeds maximum length (255)");
error errCompressedSRV = errors::New("compressed name in SRV resource data");

// packStartingCap is the default initial buffer size allocated during
// packing.
//
// The starting capacity doesn't matter too much, but most DNS responses
// Will be <= 512 bytes as it is the limit for DNS over UDP.
const int packStartingCap = 512;

// uint16Len is the length (in bytes) of a uint16.
const int uint16Len = 2;

// uint32Len is the length (in bytes) of a uint32.
const int uint32Len = 4;

// headerLen is the length (in bytes) of a DNS header.
//
// A header is comprised of 6 uint16s and no padding.
const int headerLen = 6 * uint16Len;

#define CASE_RETURN(x) \
    case x:            \
        return #x

// TypeString ...
const char* TypeString(Type typ) {
    switch (typ) {
        CASE_RETURN(TypeA);
        CASE_RETURN(TypeNS);
        CASE_RETURN(TypeCNAME);
        CASE_RETURN(TypeSOA);
        CASE_RETURN(TypePTR);
        CASE_RETURN(TypeMX);
        CASE_RETURN(TypeTXT);
        CASE_RETURN(TypeAAAA);
        CASE_RETURN(TypeSRV);
        CASE_RETURN(TypeOPT);
        CASE_RETURN(TypeWKS);
        CASE_RETURN(TypeHINFO);
        CASE_RETURN(TypeMINFO);
        CASE_RETURN(TypeAXFR);
        CASE_RETURN(TypeALL);
        default:
            return "";
    }
}

// ClassString ...
const char* ClassString(Class c) {
    switch (c) {
        CASE_RETURN(ClassINET);
        CASE_RETURN(ClassCSNET);
        CASE_RETURN(ClassCHAOS);
        CASE_RETURN(ClassHESIOD);
        CASE_RETURN(ClassANY);
        default:
            return "";
    }
}

// RCodeString ...
const char* RCodeString(RCode c) {
    switch (c) {
        CASE_RETURN(RCodeSuccess);
        CASE_RETURN(RCodeFormatError);
        CASE_RETURN(RCodeServerFailure);
        CASE_RETURN(RCodeNameError);
        CASE_RETURN(RCodeNotImplemented);
        CASE_RETURN(RCodeRefused);
        default:
            return "";
    }
}

const int headerBitQR = 1 << 15;  // query/response (response=1)
const int headerBitAA = 1 << 10;  // authoritative
const int headerBitTC = 1 << 9;   // truncated
const int headerBitRD = 1 << 8;   // recursion desired
const int headerBitRA = 1 << 7;   // recursion available
const int headerBitAD = 1 << 5;   // authentic data
const int headerBitCD = 1 << 4;   // checking disabled

error nestedError(const string& s, error err) { return errors::New(s + ": " + err->String()); }

// packUint16 appends the wire format of field to msg.
bytez<> packUint16(bytez<> msg, uint16 field) { return append(msg, byte(field >> 8), byte(field)); }

R<uint16, int, error> unpackUint16(bytez<> msg, int off) {
    if (off + uint16Len > len(msg)) {
        return {0, off, errBaseLen};
    }
    return {uint16(msg[off]) << 8 | uint16(msg[off + 1]), off + uint16Len, nil};
}

R<int, error> skipUint16(bytez<> msg, int off) {
    if (off + uint16Len > len(msg)) {
        return {off, errBaseLen};
    }
    return {off + uint16Len, nil};
}

// packType appends the wire format of field to msg.
bytez<> packType(bytez<> msg, Type field) { return packUint16(msg, uint16(field)); }

R<Type, int, error> unpackType(bytez<> msg, int off) {
    AUTO_R(t, o, err, unpackUint16(msg, off));
    return {Type(t), o, err};
}

R<int, error> skipType(bytez<> msg, int off) { return skipUint16(msg, off); }

// packClass appends the wire format of field to msg.
bytez<> packClass(bytez<> msg, Class field) { return packUint16(msg, uint16(field)); }

R<Class, int, error> unpackClass(bytez<> msg, int off) {
    AUTO_R(c, o, err, unpackUint16(msg, off));
    return {Class(c), o, err};
}

R<int, error> skipClass(bytez<> msg, int off) { return skipUint16(msg, off); }

// packUint32 appends the wire format of field to msg.
bytez<> packUint32(bytez<> msg, uint32 field) {
    return append(msg, byte(field >> 24), byte(field >> 16), byte(field >> 8), byte(field));
}

R<uint32, int, error> unpackUint32(bytez<> msg, int off) {
    if (off + uint32Len > len(msg)) {
        return {0, off, errBaseLen};
    }
    uint32 v = uint32(msg[off]) << 24 | uint32(msg[off + 1]) << 16 | uint32(msg[off + 2]) << 8 | uint32(msg[off + 3]);
    return {v, off + uint32Len, nil};
}

R<int, error> skipUint32(bytez<> msg, int off) {
    if (off + uint32Len > len(msg)) {
        return {off, errBaseLen};
    }
    return {off + uint32Len, nil};
}

// packText appends the wire format of field to msg.
R<bytez<>, error> packText(bytez<> msg, const string& field) {
    int l = len(field);
    if (l > 255) {
        return {{}, errStringTooLong};
    }
    msg = append(msg, byte(l));
    msg = append(msg, field);

    return {msg, nil};
}

R<string, int, error> unpackText(bytez<> msg, int off) {
    if (off >= len(msg)) {
        return {"", off, errBaseLen};
    }
    int beginOff = off + 1;
    int endOff = beginOff + int(msg[off]);
    if (endOff > len(msg)) {
        return {"", off, errCalcLen};
    }
    return {string(msg(beginOff, endOff)), endOff, nil};
}

// packBytes appends the wire format of field to msg.
bytez<> packBytes(bytez<> msg, bytez<> field) { return append(msg, field); }

R<int, error> unpackBytes(bytez<> msg, int off, bytez<> field) {
    int newOff = off + len(field);
    if (newOff > len(msg)) {
        return {off, errBaseLen};
    }
    copy(field, msg(off, newOff));
    return {newOff, nil};
}

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

R<int, error> header::unpack(bytez<> msg, int off) {
    auto& h = *this;
    int newOff = off;

    AUTO_R(id, newOff1, err1, unpackUint16(msg, newOff));
    h.id = id;
    if (err1 != nil) {
        return {off, nestedError("id", err1)};
    }

    AUTO_R(bits, newOff2, err2, unpackUint16(msg, newOff1));
    h.bits = bits;
    if (err2 != nil) {
        return {off, nestedError("bits", err2)};
    }

    AUTO_R(questions, newOff3, err3, unpackUint16(msg, newOff2));
    h.questions = questions;
    if (err3 != nil) {
        return {off, nestedError("questions", err3)};
    }

    AUTO_R(answers, newOff4, err4, unpackUint16(msg, newOff3));
    h.answers = answers;
    if (err4 != nil) {
        return {off, nestedError("answers", err4)};
    }

    AUTO_R(authorities, newOff5, err5, unpackUint16(msg, newOff4));
    h.authorities = authorities;
    if (err5 != nil) {
        return {off, nestedError("authorities", err5)};
    }

    AUTO_R(additionals, newOff6, err6, unpackUint16(msg, newOff5));
    h.additionals = additionals;
    newOff = newOff6;
    if (err6 != nil) {
        return {off, nestedError("additionals", err6)};
    }

    return {newOff, nil};
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
        return {oldMsg, errNonCanonicalName};
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
                return {oldMsg, errSegTooLong};
            }

            // Segments must have a non-zero length.
            if (i - begin == 0) {
                return {oldMsg, errZeroSegLen};
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

// unpack unpacks a domain name.
R<int, error> Name::unpackCompressed(bytez<> msg, int off, bool allowCompression) {
    auto& n = *this;

    // currOff is the current working offset.
    int currOff = off;

    // newOff is the offset where the next record will start. Pointers lead
    // to data that belongs to other names and thus doesn't count towards to
    // the usage of this name.
    int newOff = off;

    // ptr is the number of pointers followed.
    int ptr = 0;

    // Name is a slice representation of the name data.
    auto name = n.Data(0, 0);

__Loop:
    for (;;) {
        if (currOff >= len(msg)) {
            return {off, errBaseLen};
        }
        int c = int(msg[currOff]);
        currOff++;
        switch (c & 0xC0) {
            case 0x00: {  // String segment
                if (c == 0x00) {
                    // A zero length signals the end of the name.
                    goto __Loopend;
                }
                int endOff = currOff + c;
                if (endOff > len(msg)) {
                    return {off, errCalcLen};
                }
                // auto tmp = msg(currOff, endOff);
                // name = append(name, tmp);
                // for (int i = currOff; i < endOff; i++) {
                //     name = append(name, msg[i]);
                // }
                name = append(name, msg(currOff, endOff));
                name = append(name, '.');
                currOff = endOff;
                // println("tmp =", string(tmp), " name =", string(name));
            } break;
            case 0xC0: {  // Pointer
                if (!allowCompression) {
                    return {off, errCompressedSRV};
                }
                if (currOff >= len(msg)) {
                    return {off, errInvalidPtr};
                }
                byte c1 = msg[currOff];
                currOff++;
                if (ptr == 0) {
                    newOff = currOff;
                }
                // Don't follow too many pointers, maybe there's a loop.
                ptr++;
                if (ptr > 10) {
                    return {off, errTooManyPtr};
                }
                currOff = (c ^ 0xC0) << 8 | int(c1);
            } break;
            default:
                // Prefixes 0x80 and 0x40 are reserved.
                return {off, errReserved};
        }
    }
__Loopend:
    if (len(name) == 0) {
        name = append(name, '.');
    }
    if (len(name) > len(n.Data)) {
        return {off, errCalcLen};
    }
    n.Length = uint8(len(name));
    if (ptr == 0) {
        newOff = currOff;
    }
    return {newOff, nil};
}

R<bytez<> /*msg*/, int /*lenOff*/, error /*err*/> ResourceHeader::pack(bytez<> oldMsg, map<string, int> compression,
                                                                       int compressionOff) {
    auto& h = *this;
    AUTO_R(msg, err, h.Name.pack(oldMsg, compression, compressionOff));
    if (err != nil) {
        return {oldMsg, 0, nestedError("Name", err)};
    }
    msg = packType(msg, h.Type);
    msg = packClass(msg, h.Class);
    msg = packUint32(msg, h.TTL);
    int lenOff = len(msg);
    msg = packUint16(msg, h.Length);
    return {msg, lenOff, nil};
}

R<int, error> ResourceHeader::unpack(bytez<> msg, int off) {
    auto& h = *this;
    int newOff = off;

    AUTO_R(_newOff1, _err1, h.Name.unpack(msg, newOff));
    if (_err1 != nil) {
        return {off, nestedError("Name", _err1)};
    }

    AUTO_R(_typ, _newOff2, _err2, unpackType(msg, _newOff1));
    h.Type = _typ;
    if (_err2 != nil) {
        return {off, nestedError("Type", _err2)};
    }

    AUTO_R(_Class, _newOff3, _err3, unpackClass(msg, _newOff2));
    h.Class = _Class;
    if (_err3 != nil) {
        return {off, nestedError("Class", _err3)};
    }

    AUTO_R(_TTL, _newOff4, _err4, unpackUint32(msg, _newOff3));
    h.TTL = _TTL;
    if (_err4 != nil) {
        return {off, nestedError("TTL", _err4)};
    }

    AUTO_R(_Length, _newOff5, _err5, unpackUint16(msg, _newOff4));
    h.Length = _Length;
    if (_err5 != nil) {
        return {off, nestedError("Length", _err5)};
    }

    return {_newOff5, nil};
}

error ResourceHeader::fixLen(bytez<> msg, int lenOff, int preLen) {
    auto& h = *this;
    int conLen = len(msg) - preLen;
    if (conLen > 65535) {
        return errResTooLong;
    }

    // Fill in the length now that we know how long the content is.
    packUint16(msg(lenOff, lenOff), uint16(conLen));
    h.Length = uint16(conLen);

    return nil;
}

R<Ref<CNAMEResource>, error> unpackCNAMEResource(bytez<> msg, int off) {
    auto ret = NewRef<CNAMEResource>();
    AUTO_R(_, err, ret->CNAME.unpack(msg, off));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
}

xx::packResult MXResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    auto oldMsg = msg;
    msg = packUint16(msg, r.Pref);
    AUTO_R(_msg, err, r.MX.pack(msg, compression, compressionOff));
    if (err != nil) {
        return {oldMsg, nestedError("MXResource.MX", err)};
    }
    return {_msg, nil};
}

R<Ref<MXResource>, error> unpackMXResource(bytez<> msg, int off) {
    auto ret = NewRef<MXResource>();
    AUTO_R(_, err, ret->MX.unpack(msg, off));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
}

R<Ref<NSResource>, error> unpackNSResource(bytez<> msg, int off) {
    auto ret = NewRef<NSResource>();
    AUTO_R(_, err, ret->NS.unpack(msg, off));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
}

R<Ref<PTRResource>, error> unpackPTRResource(bytez<> msg, int off) {
    auto ret = NewRef<PTRResource>();
    AUTO_R(_, err, ret->PTR.unpack(msg, off));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
}

xx::packResult SOAResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    auto oldMsg = msg;
    AUTO_R(_msg1, _err1, r.NS.pack(msg, compression, compressionOff));
    if (_err1 != nil) {
        return {oldMsg, nestedError("SOAResource.NS", _err1)};
    }
    AUTO_R(_msg2, _err2, r.MBox.pack(_msg1, compression, compressionOff));
    if (_err2 != nil) {
        return {oldMsg, nestedError("SOAResource.MBox", _err2)};
    }
    msg = packUint32(_msg2, r.Serial);
    msg = packUint32(msg, r.Refresh);
    msg = packUint32(msg, r.Retry);
    msg = packUint32(msg, r.Expire);
    return {packUint32(msg, r.MinTTL), nil};
}

R<Ref<SOAResource>, error> unpackSOAResource(bytez<> msg, int off) {
    auto ret = NewRef<SOAResource>();

    AUTO_R(off1, err1, ret->NS.unpack(msg, off));
    if (err1 != nil) {
        return {nil, nestedError("NS", err1)};
    }

    AUTO_R(off2, err2, ret->MBox.unpack(msg, off1));
    if (err2 != nil) {
        return {nil, nestedError("MBox", err2)};
    }

    AUTO_R(serial, off3, err3, unpackUint32(msg, off2));
    if (err3 != nil) {
        return {nil, nestedError("Serial", err3)};
    }

    AUTO_R(refresh, off4, err4, unpackUint32(msg, off3));
    if (err4 != nil) {
        return {nil, nestedError("Refresh", err4)};
    }

    AUTO_R(retry, off5, err5, unpackUint32(msg, off4));
    if (err5 != nil) {
        return {nil, nestedError("Retry", err5)};
    }

    AUTO_R(expire, off6, err6, unpackUint32(msg, off5));
    if (err6 != nil) {
        return {nil, nestedError("Expire", err6)};
    }

    AUTO_R(minTTL, _, err7, unpackUint32(msg, off6));
    if (err7 != nil) {
        return {nil, nestedError("MinTTL", err7)};
    }

    ret->Serial = serial;
    ret->Refresh = refresh;
    ret->Expire = expire;
    ret->MinTTL = minTTL;
    return {ret, nil};
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

R<Ref<TXTResource>, error> unpackTXTResource(bytez<> msg, int off, uint16 length) {
    auto ret = NewRef<TXTResource>();
    ret->TXT = make<string>(0, 1);
    for (uint16 n = uint16(0); n < length;) {
        AUTO_R(t, _off, err, unpackText(msg, off));
        off = _off;
        if (err != nil) {
            return {nil, nestedError("text", err)};
        }
        // Check if we got too many bytes.
        if (length - n < uint16(len(t)) + 1) {
            return {nil, errCalcLen};
        }
        n += uint16(len(t)) + 1;
        ret->TXT = append(ret->TXT, t);
    }

    return {ret, nil};
}

xx::packResult SRVResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    auto oldMsg = msg;
    msg = packUint16(msg, r.Priority);
    msg = packUint16(msg, r.Weight);
    msg = packUint16(msg, r.Port);
    AUTO_R(_msg, err, r.Target.pack(msg, {}, compressionOff));
    if (err != nil) {
        return {oldMsg, nestedError("SRVResource.Target", err)};
    }
    return {_msg, nil};
}

R<Ref<SRVResource>, error> unpackSRVResource(bytez<> msg, int off) {
    AUTO_R(priority, _off1, err1, unpackUint16(msg, off));
    if (err1 != nil) {
        return {nil, nestedError("Priority", err1)};
    }

    AUTO_R(weight, _off2, err2, unpackUint16(msg, _off1));
    if (err2 != nil) {
        return {nil, nestedError("Weight", err2)};
    }

    AUTO_R(port, _off3, err3, unpackUint16(msg, _off2));
    if (err3 != nil) {
        return {nil, nestedError("Port", err3)};
    }

    auto ret = NewRef<SRVResource>();
    AUTO_R(_, err, ret->Target.unpackCompressed(msg, _off3, false /* allowCompression */));
    if (err != nil) {
        return {nil, nestedError("Target", err)};
    }

    ret->Priority = priority;
    ret->Weight = weight;
    ret->Port = port;
    return {ret, nil};
}

xx::packResult AResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    return {packBytes(msg, r.A(0, -1)), nil};
}

R<Ref<AResource>, error> unpackAResource(bytez<> msg, int off) {
    auto ret = NewRef<AResource>();
    AUTO_R(_, err, unpackBytes(msg, off, ret->A(0, -1)));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
}

xx::packResult AAAAResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    return {packBytes(msg, r.AAAA(0, -1)), nil};
}

R<Ref<AAAAResource>, error> unpackAAAAResource(bytez<> msg, int off) {
    auto ret = NewRef<AAAAResource>();
    AUTO_R(_, err, unpackBytes(msg, off, ret->AAAA(0, -1)));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
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

R<Ref<OPTResource>, error> unpackOPTResource(bytez<> msg, int off, uint16 length) {
    auto ret = NewRef<OPTResource>();
    for (int oldOff = off; off < oldOff + int(length);) {
        Option o;
        AUTO_R(_Code, _off1, er1, unpackUint16(msg, off));
        o.Code = _Code;
        off = _off1;
        if (er1 != nil) {
            return {nil, nestedError("Code", er1)};
        }
        AUTO_R(l, _off2, er2, unpackUint16(msg, off));
        off = _off2;
        if (er2 != nil) {
            return {nil, nestedError("Data", er2)};
        }
        o.Data = make(l);
        if (copy(o.Data, msg(off)) != int(l)) {
            return {nil, nestedError("Data", errCalcLen)};
        }
        off += int(l);
        ret->Options = append(ret->Options, o);
    }
    return {ret, nil};
}

xx::packResult UnknownResource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    return {packBytes(msg, r.Data(0, -1)), nil};
}

R<Ref<UnknownResource>, error> unpackUnknownResource(Type recordType, bytez<> msg, int off, uint16 length) {
    auto parsed = NewRef<UnknownResource>();
    parsed->Type = recordType;
    parsed->Data = make(length);
    AUTO_R(_, err, unpackBytes(msg, off, parsed->Data));
    if (err != nil) {
        return {nil, err};
    }
    return {parsed, nil};
}

R<string, Ref<ResourceBody>, error> _unpackResourceBody(bytez<> msg, int off, ResourceHeader hdr) {
    switch (hdr.Type) {
        case TypeA: {
            AUTO_R(rb, er, unpackAResource(msg, off));
            return {"A", rb, er};
        }
        case TypeNS: {
            AUTO_R(rb, er, unpackNSResource(msg, off));
            return {"NS", rb, er};
        }
        case TypeCNAME: {
            AUTO_R(rb, er, unpackCNAMEResource(msg, off));
            return {"CNAME", rb, er};
        }
        case TypeSOA: {
            AUTO_R(rb, er, unpackSOAResource(msg, off));
            return {"SOA", rb, er};
        }
        case TypePTR: {
            AUTO_R(rb, er, unpackPTRResource(msg, off));
            return {"PTR", rb, er};
        }
        case TypeMX: {
            AUTO_R(rb, er, unpackMXResource(msg, off));
            return {"MX", rb, er};
        }
        case TypeTXT: {
            AUTO_R(rb, er, unpackTXTResource(msg, off, hdr.Length));
            return {"TXT", rb, er};
        }
        case TypeAAAA: {
            AUTO_R(rb, er, unpackAAAAResource(msg, off));
            return {"AAAA", rb, er};
        }
        case TypeSRV: {
            AUTO_R(rb, er, unpackSRVResource(msg, off));
            return {"SRV", rb, er};
        }
        case TypeOPT: {
            AUTO_R(rb, er, unpackOPTResource(msg, off, hdr.Length));
            return {"OPT", rb, er};
        }
        default: {
            AUTO_R(rb, er, unpackUnknownResource(hdr.Type, msg, off, hdr.Length));
            return {"Unknown", rb, er};
        }
    }
}

R<Ref<ResourceBody>, int, error> unpackResourceBody(bytez<> msg, int off, ResourceHeader hdr) {
    AUTO_R(name, r, err, _unpackResourceBody(msg, off, hdr));
    if (err != nil) {
        return {nil, off, nestedError(name + " record", err)};
    }
    return {r, off + int(hdr.Length), nil};
}

// Start parses the header and enables the parsing of Questions.
R<Header, error> Parser::Start(bytez<> msg) {
    auto& p = *this;
    p.msg = msg;
    AUTO_R(off, err, p.header.unpack(msg, 0));
    p.off = off;
    if (err != nil) {
        return {Header{}, nestedError("unpacking header", err)};
    }
    p.section = xx::sectionQuestions;
    return {p.header.Header(), nil};
}

error Parser::checkAdvance(xx::section sec) {
    auto& p = *this;
    if (p.section < sec) {
        return ErrNotStarted;
    }
    if (p.section > sec) {
        return ErrSectionDone;
    }
    p.resHeaderValid = false;
    if (p.index == int(p.header.count(sec))) {
        p.index = 0;
        p.section++;
        return ErrSectionDone;
    }
    return nil;
}

namespace xx {
const char* sectionNames(section sec) {
    switch (sec) {
        case sectionHeader:
            return "header";
        case sectionQuestions:
            return "Question";
        case sectionAnswers:
            return "Answer";
        case sectionAuthorities:
            return "Authority";
        case sectionAdditionals:
            return "Additional";
    }
    return "";
}
}  // namespace xx

R<Resource, error> Parser::resource(xx::section sec) {
    auto& p = *this;
    Resource r;
    AUTO_R(_hdr, err, p.resourceHeader(sec));
    r.Header = _hdr;
    if (err != nil) {
        return {r, err};
    }
    p.resHeaderValid = false;
    AUTO_R(_body, _off, er2, unpackResourceBody(p.msg, p.off, r.Header));
    r.Body = _body;
    p.off = _off;
    if (er2 != nil) {
        return {Resource{}, nestedError(string("unpacking ") + xx::sectionNames(sec), er2)};
    }
    p.index++;
    return {r, nil};
}

R<ResourceHeader, error> Parser::resourceHeader(xx::section sec) {
    auto& p = *this;
    if (p.resHeaderValid) {
        return {p.resHeader, nil};
    }
    auto err = p.checkAdvance(sec);
    if (err != nil) {
        return {ResourceHeader{}, err};
    }
    ResourceHeader hdr;
    AUTO_R(off, err2, hdr.unpack(p.msg, p.off));
    if (err2 != nil) {
        return {ResourceHeader{}, err2};
    }
    p.resHeaderValid = true;
    p.resHeader = hdr;
    p.off = off;
    return {hdr, nil};
}

R<int, error> skipName(bytez<> msg, int off) {
    // newOff is the offset where the next record will start. Pointers lead
    // to data that belongs to other names and thus doesn't count towards to
    // the usage of this name.
    int newOff = off;

    for (;;) {
        if (newOff >= len(msg)) {
            return {off, errBaseLen};
        }
        int c = int(msg[newOff]);
        newOff++;
        switch (c & 0xC0) {
            case 0x00:
                if (c == 0x00) {
                    // A zero length signals the end of the name.
                    return {newOff, nil};
                }
                // literal string
                newOff += c;
                if (newOff > len(msg)) {
                    return {off, errCalcLen};
                }
                break;
            case 0xC0:
                // Pointer to somewhere else in msg.

                // Pointers are two bytes.
                newOff++;

                // Don't follow the pointer as the data here has ended.
                return {newOff, nil};
            default:
                // Prefixes 0x80 and 0x40 are reserved.
                return {off, errReserved};
        }
    }

    return {newOff, nil};
}

R<int, error> skipResource(bytez<> msg, int off) {
    AUTO_R(newOff, err, skipName(msg, off));
    if (err != nil) {
        return {off, nestedError("Name", err)};
    }

    AUTO_R(_newOff1, err1, skipType(msg, newOff));
    if (err1 != nil) {
        return {off, nestedError("Type", err1)};
    }

    AUTO_R(_newOff2, err2, skipClass(msg, _newOff1));
    if (err2 != nil) {
        return {off, nestedError("Class", err2)};
    }

    AUTO_R(_newOff3, err3, skipUint32(msg, _newOff2));
    if (err3 != nil) {
        return {off, nestedError("TTL", err3)};
    }

    AUTO_R(length, _newOff4, err4, unpackUint16(msg, _newOff3));
    if (err4 != nil) {
        return {off, nestedError("Length", err4)};
    }
    _newOff4 += int(length);
    if (_newOff4 > len(msg)) {
        return {off, errResourceLen};
    }

    return {_newOff4, nil};
}

error Parser::skipResource(xx::section sec) {
    auto& p = *this;
    if (p.resHeaderValid) {
        int newOff = p.off + int(p.resHeader.Length);
        if (newOff > len(p.msg)) {
            return errResourceLen;
        }
        p.off = newOff;
        p.resHeaderValid = false;
        p.index++;
        return nil;
    }
    auto err = p.checkAdvance(sec);
    if (err != nil) {
        return err;
    }
    AUTO_R(_off, err2, dnsmessage::skipResource(p.msg, p.off));
    p.off = _off;
    if (err2 != nil) {
        return nestedError(string("skipping: ") + xx::sectionNames(sec), err2);
    }
    p.index++;
    return nil;
}

// AllQuestions parses all Questions.
R<slice<Question>, error> Parser::AllQuestions() {
    auto& p = *this;
    // Multiple questions are valid according to the spec,
    // but servers don't actually support them. There will
    // be at most one question here.
    //
    // Do not pre-allocate based on info in p.header, since
    // the data is untrusted.
    auto qs = make<dnsmessage::Question>();
    for (;;) {
        AUTO_R(q, err, p.Question());
        if (err == ErrSectionDone) {
            return {qs, nil};
        }
        if (err != nil) {
            return {{}, err};
        }
        qs = append(qs, q);
    }
}

// Question parses a single Question.
R<Question, error> Parser::Question() {
    auto& p = *this;

    auto err = p.checkAdvance(xx::sectionQuestions);
    if (err != nil) {
        return {{}, err};
    }

    dnsmessage::Question ret;

    AUTO_R(off, err0, ret.Name.unpack(p.msg, p.off));
    if (err0 != nil) {
        return {{}, nestedError("unpacking Question.Name", err0)};
    }

    AUTO_R(typ, _off1, err1, unpackType(p.msg, off));
    off = _off1;
    ret.Type = typ;
    if (err1 != nil) {
        return {{}, nestedError("unpacking Question.Type", err1)};
    }

    AUTO_R(clasz, _off2, err2, unpackClass(p.msg, off));
    off = _off2;
    ret.Class = clasz;
    if (err2 != nil) {
        return {{}, nestedError("unpacking Question.Class", err2)};
    }

    p.off = off;
    p.index++;
    return {ret, nil};
}

// SkipQuestion skips a single Question.
error Parser::SkipQuestion() {
    auto& p = *this;

    auto err = p.checkAdvance(xx::sectionQuestions);
    if (err != nil) {
        return err;
    }

    AUTO_R(off1, err1, skipName(p.msg, p.off));
    if (err1 != nil) {
        return nestedError("skipping Question Name", err1);
    }

    AUTO_R(off2, err2, skipType(p.msg, off1));
    if (err2 != nil) {
        return nestedError("skipping Question Type", err2);
    }

    AUTO_R(off3, err3, skipClass(p.msg, off2));
    if (err3 != nil) {
        return nestedError("skipping Question Class", err3);
    }

    p.off = off3;
    p.index++;
    return nil;
}

// SkipAllQuestions skips all Questions.
error Parser::SkipAllQuestions() {
    auto& p = *this;
    for (;;) {
        auto err = p.SkipQuestion();
        if (err == ErrSectionDone) {
            return nil;
        } else if (err != nil) {
            return err;
        }
    }
}

// AllAnswers parses all Answer Resources.
R<slice<Resource>, error> Parser::AllAnswers() {
    auto& p = *this;
    // The most common query is for A/AAAA, which usually returns
    // a handful of IPs.
    //
    // Pre-allocate up to a certain limit, since p.header is
    // untrusted data.
    int n = int(p.header.answers);
    if (n > 20) {
        n = 20;
    }
    auto as = make<Resource>(0, n);
    for (;;) {
        AUTO_R(a, err, p.Answer());
        if (err == ErrSectionDone) {
            return {as, nil};
        }
        if (err != nil) {
            return {{}, err};
        }
        as = append(as, a);
    }
}

// SkipAllAnswers skips all Answer Resources.
error Parser::SkipAllAnswers() {
    auto& p = *this;
    for (;;) {
        auto err = p.SkipAnswer();
        if (err == ErrSectionDone) {
            return nil;
        } else if (err != nil) {
            return err;
        }
    }
}

// AuthorityHeader parses a sing
// AllAuthorities parses all Authority Resources.
R<slice<Resource>, error> Parser::AllAuthorities() {
    auto& p = *this;
    // Authorities contains SOA in case of NXDOMAIN and friends,
    // otherwise it is empty.
    //
    // Pre-allocate up to a certain limit, since p.header is
    // untrusted data.
    int n = int(p.header.authorities);
    if (n > 10) {
        n = 10;
    }
    auto as = make<Resource>(0, n);
    for (;;) {
        AUTO_R(a, err, p.Authority());
        if (err == ErrSectionDone) {
            return {as, nil};
        }
        if (err != nil) {
            return {{}, err};
        }
        as = append(as, a);
    }
}

// SkipAllAuthorities skips all Authority Resources.
error Parser::SkipAllAuthorities() {
    auto& p = *this;
    for (;;) {
        auto err = p.SkipAuthority();
        if (err == ErrSectionDone) {
            return nil;
        } else if (err != nil) {
            return err;
        }
    }
}

// AllAdditionals parses all Additional Resources.
R<slice<Resource>, error> Parser::AllAdditionals() {
    auto& p = *this;
    // Additionals usually contain OPT, and sometimes A/AAAA
    // glue records.
    //
    // Pre-allocate up to a certain limit, since p.header is
    // untrusted data.
    int n = int(p.header.additionals);
    if (n > 10) {
        n = 10;
    }
    auto as = make<Resource>(0, n);
    for (;;) {
        AUTO_R(a, err, p.Additional());
        if (err == ErrSectionDone) {
            return {as, nil};
        }
        if (err != nil) {
            return {{}, err};
        }
        as = append(as, a);
    }
}

// SkipAllAdditionals skips all Additional Resources.
error Parser::SkipAllAdditionals() {
    auto& p = *this;
    for (;;) {
        auto err = p.SkipAdditional();
        if (err == ErrSectionDone) {
            return nil;
        } else if (err != nil) {
            return err;
        }
    }
}

// CNAMEResource parses a single CNAMEResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<CNAMEResource>, error> Parser::CNAMEResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeCNAME) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackCNAMEResource(p.msg, p.off));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// MXResource parses a single MXResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<MXResource>, error> Parser::MXResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeMX) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackMXResource(p.msg, p.off));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// NSResource parses a single NSResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<NSResource>, error> Parser::NSResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeNS) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackNSResource(p.msg, p.off));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// PTRResource parses a single PTRResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<PTRResource>, error> Parser::PTRResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypePTR) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackPTRResource(p.msg, p.off));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// SOAResource parses a single SOAResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<SOAResource>, error> Parser::SOAResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeSOA) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackSOAResource(p.msg, p.off));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// TXTResource parses a single TXTResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<TXTResource>, error> Parser::TXTResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeTXT) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackTXTResource(p.msg, p.off, p.resHeader.Length));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// SRVResource parses a single SRVResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<SRVResource>, error> Parser::SRVResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeSRV) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackSRVResource(p.msg, p.off));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// AResource parses a single AResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<AResource>, error> Parser::AResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeA) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackAResource(p.msg, p.off));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// AAAAResource parses a single AAAAResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<AAAAResource>, error> Parser::AAAAResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeAAAA) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackAAAAResource(p.msg, p.off));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// OPTResource parses a single OPTResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<OPTResource>, error> Parser::OPTResource() {
    auto& p = *this;
    if (!p.resHeaderValid || p.resHeader.Type != TypeOPT) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackOPTResource(p.msg, p.off, p.resHeader.Length));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// UnknownResource parses a single UnknownResource.
//
// One of the XXXHeader methods must have been called before calling this
// method.
R<Ref<UnknownResource>, error> Parser::UnknownResource() {
    auto& p = *this;
    if (!p.resHeaderValid) {
        return {nil, ErrNotStarted};
    }
    AUTO_R(r, err, unpackUnknownResource(p.resHeader.Type, p.msg, p.off, p.resHeader.Length));
    if (err != nil) {
        return {nil, err};
    }
    p.off += int(p.resHeader.Length);
    p.resHeaderValid = false;
    p.index++;
    return {r, nil};
}

// Unpack parses a full Message.
error Message::Unpack(bytez<> msg) {
    auto& m = *this;
    Parser p;

    AUTO_R(hdr, err1, p.Start(msg));
    m.Header = hdr;
    if (err1 != nil) {
        return err1;
    }

    AUTO_R(ques, err2, p.AllQuestions());
    m.Questions = ques;
    if (err2 != nil) {
        return err2;
    }

    AUTO_R(anws, err3, p.AllAnswers());
    m.Answers = anws;
    if (err3 != nil) {
        return err3;
    }

    AUTO_R(auth, err4, p.AllAuthorities());
    m.Authorities = auth;
    if (err4 != nil) {
        return err4;
    }

    AUTO_R(addi, err5, p.AllAdditionals());
    m.Additionals = addi;
    if (err5 != nil) {
        return err5;
    }

    return nil;
}

// pack appends the wire format of the Question to msg.
xx::packResult Question::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& q = *this;
    AUTO_R(_msg, err, q.Name.pack(msg, compression, compressionOff));
    msg = _msg;
    if (err != nil) {
        return {msg, nestedError("Name", err)};
    }
    msg = packType(msg, q.Type);
    return {packClass(msg, q.Class), nil};
}

// pack appends the wire format of the Question to msg.
xx::packResult Resource::pack(bytez<> msg, map<string, int> compression, int compressionOff) {
    auto& r = *this;
    if (r.Body == nil) {
        return {msg, errNilResouceBody};
    }

    auto oldMsg = msg;
    r.Header.Type = r.Body->realType();

    AUTO_R(_msg1, lenOff, err1, r.Header.pack(msg, compression, compressionOff));
    msg = _msg1;
    if (err1 != nil) {
        return {msg, nestedError("ResourceHeader", err1)};
    }

    int preLen = len(msg);
    AUTO_R(_msg2, err2, r.Body->pack(msg, compression, compressionOff));
    msg = _msg2;
    if (err2 != nil) {
        return {msg, nestedError("content", err2)};
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
    if (len(m.Questions) > 65535) {
        return {{}, errTooManyQuestions};
    }
    if (len(m.Answers) > 65535) {
        return {{}, errTooManyAnswers};
    }
    if (len(m.Authorities) > 65535) {
        return {{}, errTooManyAuthorities};
    }
    if (len(m.Additionals) > 65535) {
        return {{}, errTooManyAdditionals};
    }

    xx::header h;
    AUTO_R(id, bits, m.Header.pack());
    h.id = id;
    h.bits = bits;

    h.questions = uint16(len(m.Questions));
    h.answers = uint16(len(m.Answers));
    h.authorities = uint16(len(m.Authorities));

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
            return {{}, nestedError("packing Question", _err)};
        }
        msg = _msg;
    }
    for (int i = 0; i < len(m.Answers); i++) {
        AUTO_R(_msg, _err, m.Answers[i].pack(msg, compression, compressionOff));
        if (_err != nil) {
            return {{}, nestedError("packing Answer", _err)};
        }
        msg = _msg;
    }
    for (int i = 0; i < len(m.Authorities); i++) {
        AUTO_R(_msg, _err, m.Authorities[i].pack(msg, compression, compressionOff));
        if (_err != nil) {
            return {{}, nestedError("packing Authority", _err)};
        }
        msg = _msg;
    }
    for (int i = 0; i < len(m.Additionals); i++) {
        AUTO_R(_msg, _err, m.Additionals[i].pack(msg, compression, compressionOff));
        if (_err != nil) {
            return {{}, nestedError("packing Additional", _err)};
        }
        msg = _msg;
    }

    return {msg, nil};
}

}  // namespace dnsmessage
}  // namespace gx

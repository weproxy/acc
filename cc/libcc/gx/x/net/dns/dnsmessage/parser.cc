//
// weproxy@foxmail.com 2022/10/23
//

#include "dnsmessage.h"

namespace gx {
namespace dnsmessage {

////////////////////////////////////////////////////////////////////////////////

// uint16Len is the length (in bytes) of a uint16.
static const int uint16Len = 2;

// uint32Len is the length (in bytes) of a uint32.
static const int uint32Len = 4;

////////////////////////////////////////////////////////////////////////////////

static R<uint16, int, error> unpackUint16(bytez<> msg, int off) {
    if (off + uint16Len > len(msg)) {
        return {0, off, xx::errBaseLen};
    }
    return {uint16(msg[off]) << 8 | uint16(msg[off + 1]), off + uint16Len, nil};
}

static R<int, error> skipUint16(bytez<> msg, int off) {
    if (off + uint16Len > len(msg)) {
        return {off, xx::errBaseLen};
    }
    return {off + uint16Len, nil};
}

static R<uint32, int, error> unpackUint32(bytez<> msg, int off) {
    if (off + uint32Len > len(msg)) {
        return {0, off, xx::errBaseLen};
    }
    uint32 v = uint32(msg[off]) << 24 | uint32(msg[off + 1]) << 16 | uint32(msg[off + 2]) << 8 | uint32(msg[off + 3]);
    return {v, off + uint32Len, nil};
}

static R<int, error> skipUint32(bytez<> msg, int off) {
    if (off + uint32Len > len(msg)) {
        return {off, xx::errBaseLen};
    }
    return {off + uint32Len, nil};
}

static R<Type, int, error> unpackType(bytez<> msg, int off) {
    AUTO_R(t, o, err, unpackUint16(msg, off));
    return {Type(t), o, err};
}

static R<int, error> skipType(bytez<> msg, int off) { return skipUint16(msg, off); }

static R<Class, int, error> unpackClass(bytez<> msg, int off) {
    AUTO_R(c, o, err, unpackUint16(msg, off));
    return {Class(c), o, err};
}

static R<int, error> skipClass(bytez<> msg, int off) { return skipUint16(msg, off); }

static R<string, int, error> unpackText(bytez<> msg, int off) {
    if (off >= len(msg)) {
        return {"", off, xx::errBaseLen};
    }
    int beginOff = off + 1;
    int endOff = beginOff + int(msg[off]);
    if (endOff > len(msg)) {
        return {"", off, xx::errCalcLen};
    }
    return {string(msg(beginOff, endOff)), endOff, nil};
}

static R<int, error> unpackBytes(bytez<> msg, int off, bytez<> field) {
    int newOff = off + len(field);
    if (newOff > len(msg)) {
        return {off, xx::errBaseLen};
    }
    copy(field, msg(off, newOff));
    return {newOff, nil};
}

static R<int, error> skipName(bytez<> msg, int off) {
    // newOff is the offset where the next record will start. Pointers lead
    // to data that belongs to other names and thus doesn't count towards to
    // the usage of this name.
    int newOff = off;

    for (;;) {
        if (newOff >= len(msg)) {
            return {off, xx::errBaseLen};
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
                    return {off, xx::errCalcLen};
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
                return {off, xx::errReserved};
        }
    }

    return {newOff, nil};
}

namespace xx {
static R<int, error> skipResource(bytez<> msg, int off) {
    AUTO_R(newOff, err, skipName(msg, off));
    if (err != nil) {
        return {off, xx::nestedError("Name", err)};
    }

    AUTO_R(_newOff1, err1, skipType(msg, newOff));
    if (err1 != nil) {
        return {off, xx::nestedError("Type", err1)};
    }

    AUTO_R(_newOff2, err2, skipClass(msg, _newOff1));
    if (err2 != nil) {
        return {off, xx::nestedError("Class", err2)};
    }

    AUTO_R(_newOff3, err3, skipUint32(msg, _newOff2));
    if (err3 != nil) {
        return {off, xx::nestedError("TTL", err3)};
    }

    AUTO_R(length, _newOff4, err4, unpackUint16(msg, _newOff3));
    if (err4 != nil) {
        return {off, xx::nestedError("Length", err4)};
    }
    _newOff4 += int(length);
    if (_newOff4 > len(msg)) {
        return {off, xx::errResourceLen};
    }

    return {_newOff4, nil};
}

R<int, error> header::unpack(bytez<> msg, int off) {
    auto& h = *this;
    int newOff = off;

    AUTO_R(id, newOff1, err1, unpackUint16(msg, newOff));
    h.id = id;
    if (err1 != nil) {
        return {off, xx::nestedError("id", err1)};
    }

    AUTO_R(bits, newOff2, err2, unpackUint16(msg, newOff1));
    h.bits = bits;
    if (err2 != nil) {
        return {off, xx::nestedError("bits", err2)};
    }

    AUTO_R(questions, newOff3, err3, unpackUint16(msg, newOff2));
    h.questions = questions;
    if (err3 != nil) {
        return {off, xx::nestedError("questions", err3)};
    }

    AUTO_R(answers, newOff4, err4, unpackUint16(msg, newOff3));
    h.answers = answers;
    if (err4 != nil) {
        return {off, xx::nestedError("answers", err4)};
    }

    AUTO_R(authorities, newOff5, err5, unpackUint16(msg, newOff4));
    h.authorities = authorities;
    if (err5 != nil) {
        return {off, xx::nestedError("authorities", err5)};
    }

    AUTO_R(additionals, newOff6, err6, unpackUint16(msg, newOff5));
    h.additionals = additionals;
    newOff = newOff6;
    if (err6 != nil) {
        return {off, xx::nestedError("additionals", err6)};
    }

    return {newOff, nil};
}
}  // namespace xx

////////////////////////////////////////////////////////////////////////////////

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
            return {off, xx::errBaseLen};
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
                    return {off, xx::errCalcLen};
                }
                name = append(name, msg(currOff, endOff));
                name = append(name, '.');
                currOff = endOff;
                break;
            }
            case 0xC0: {  // Pointer
                if (!allowCompression) {
                    return {off, xx::errCompressedSRV};
                }
                if (currOff >= len(msg)) {
                    return {off, xx::errInvalidPtr};
                }
                byte c1 = msg[currOff];
                currOff++;
                if (ptr == 0) {
                    newOff = currOff;
                }
                // Don't follow too many pointers, maybe there's a loop.
                ptr++;
                if (ptr > 10) {
                    return {off, xx::errTooManyPtr};
                }
                currOff = (c ^ 0xC0) << 8 | int(c1);
                break;
            }
            default:
                // Prefixes 0x80 and 0x40 are reserved.
                return {off, xx::errReserved};
        }
    }
__Loopend:
    if (len(name) == 0) {
        name = append(name, '.');
    }
    if (len(name) > len(n.Data)) {
        return {off, xx::errCalcLen};
    }
    n.Length = uint8(len(name));
    if (ptr == 0) {
        newOff = currOff;
    }
    return {newOff, nil};
}

R<int, error> ResourceHeader::unpack(bytez<> msg, int off) {
    auto& h = *this;
    int newOff = off;

    AUTO_R(_newOff1, _err1, h.Name.unpack(msg, newOff));
    if (_err1 != nil) {
        return {off, xx::nestedError("Name", _err1)};
    }

    AUTO_R(_typ, _newOff2, _err2, unpackType(msg, _newOff1));
    h.Type = _typ;
    if (_err2 != nil) {
        return {off, xx::nestedError("Type", _err2)};
    }

    AUTO_R(_Class, _newOff3, _err3, unpackClass(msg, _newOff2));
    h.Class = _Class;
    if (_err3 != nil) {
        return {off, xx::nestedError("Class", _err3)};
    }

    AUTO_R(_TTL, _newOff4, _err4, unpackUint32(msg, _newOff3));
    h.TTL = _TTL;
    if (_err4 != nil) {
        return {off, xx::nestedError("TTL", _err4)};
    }

    AUTO_R(_Length, _newOff5, _err5, unpackUint16(msg, _newOff4));
    h.Length = _Length;
    if (_err5 != nil) {
        return {off, xx::nestedError("Length", _err5)};
    }

    return {_newOff5, nil};
}

R<Ref<CNAMEResource>, error> unpackCNAMEResource(bytez<> msg, int off) {
    auto ret = NewRef<CNAMEResource>();
    AUTO_R(_, err, ret->CNAME.unpack(msg, off));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
}

R<Ref<TXTResource>, error> unpackTXTResource(bytez<> msg, int off, uint16 length) {
    auto ret = NewRef<TXTResource>();
    ret->TXT = make<string>(0, 1);
    for (uint16 n = uint16(0); n < length;) {
        AUTO_R(t, _off, err, unpackText(msg, off));
        off = _off;
        if (err != nil) {
            return {nil, xx::nestedError("text", err)};
        }
        // Check if we got too many bytes.
        if (length - n < uint16(len(t)) + 1) {
            return {nil, xx::errCalcLen};
        }
        n += uint16(len(t)) + 1;
        ret->TXT = append(ret->TXT, t);
    }

    return {ret, nil};
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

R<Ref<SOAResource>, error> unpackSOAResource(bytez<> msg, int off) {
    auto ret = NewRef<SOAResource>();

    AUTO_R(off1, err1, ret->NS.unpack(msg, off));
    if (err1 != nil) {
        return {nil, xx::nestedError("NS", err1)};
    }

    AUTO_R(off2, err2, ret->MBox.unpack(msg, off1));
    if (err2 != nil) {
        return {nil, xx::nestedError("MBox", err2)};
    }

    AUTO_R(serial, off3, err3, unpackUint32(msg, off2));
    if (err3 != nil) {
        return {nil, xx::nestedError("Serial", err3)};
    }

    AUTO_R(refresh, off4, err4, unpackUint32(msg, off3));
    if (err4 != nil) {
        return {nil, xx::nestedError("Refresh", err4)};
    }

    AUTO_R(retry, off5, err5, unpackUint32(msg, off4));
    if (err5 != nil) {
        return {nil, xx::nestedError("Retry", err5)};
    }

    AUTO_R(expire, off6, err6, unpackUint32(msg, off5));
    if (err6 != nil) {
        return {nil, xx::nestedError("Expire", err6)};
    }

    AUTO_R(minTTL, _, err7, unpackUint32(msg, off6));
    if (err7 != nil) {
        return {nil, xx::nestedError("MinTTL", err7)};
    }

    ret->Serial = serial;
    ret->Refresh = refresh;
    ret->Expire = expire;
    ret->MinTTL = minTTL;
    return {ret, nil};
}

R<Ref<SRVResource>, error> unpackSRVResource(bytez<> msg, int off) {
    AUTO_R(priority, _off1, err1, unpackUint16(msg, off));
    if (err1 != nil) {
        return {nil, xx::nestedError("Priority", err1)};
    }

    AUTO_R(weight, _off2, err2, unpackUint16(msg, _off1));
    if (err2 != nil) {
        return {nil, xx::nestedError("Weight", err2)};
    }

    AUTO_R(port, _off3, err3, unpackUint16(msg, _off2));
    if (err3 != nil) {
        return {nil, xx::nestedError("Port", err3)};
    }

    auto ret = NewRef<SRVResource>();
    AUTO_R(_, err, ret->Target.unpackCompressed(msg, _off3, false /* allowCompression */));
    if (err != nil) {
        return {nil, xx::nestedError("Target", err)};
    }

    ret->Priority = priority;
    ret->Weight = weight;
    ret->Port = port;
    return {ret, nil};
}

R<Ref<AResource>, error> unpackAResource(bytez<> msg, int off) {
    auto ret = NewRef<AResource>();
    AUTO_R(_, err, unpackBytes(msg, off, ret->A(0, -1)));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
}

R<Ref<AAAAResource>, error> unpackAAAAResource(bytez<> msg, int off) {
    auto ret = NewRef<AAAAResource>();
    AUTO_R(_, err, unpackBytes(msg, off, ret->AAAA(0, -1)));
    if (err != nil) {
        return {nil, err};
    }
    return {ret, nil};
}

R<Ref<OPTResource>, error> unpackOPTResource(bytez<> msg, int off, uint16 length) {
    auto ret = NewRef<OPTResource>();
    for (int oldOff = off; off < oldOff + int(length);) {
        Option o;
        AUTO_R(_Code, _off1, er1, unpackUint16(msg, off));
        o.Code = _Code;
        off = _off1;
        if (er1 != nil) {
            return {nil, xx::nestedError("Code", er1)};
        }
        AUTO_R(l, _off2, er2, unpackUint16(msg, off));
        off = _off2;
        if (er2 != nil) {
            return {nil, xx::nestedError("Data", er2)};
        }
        o.Data = make(l);
        if (copy(o.Data, msg(off)) != int(l)) {
            return {nil, xx::nestedError("Data", xx::errCalcLen)};
        }
        off += int(l);
        ret->Options = append(ret->Options, o);
    }
    return {ret, nil};
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

R<string, ResourceBody, error> _unpackResourceBody(bytez<> msg, int off, ResourceHeader hdr) {
    switch (hdr.Type) {
        case Type::A: {
            AUTO_R(rb, er, unpackAResource(msg, off));
            return {"A", rb, er};
        }
        case Type::NS: {
            AUTO_R(rb, er, unpackNSResource(msg, off));
            return {"NS", rb, er};
        }
        case Type::CNAME: {
            AUTO_R(rb, er, unpackCNAMEResource(msg, off));
            return {"CNAME", rb, er};
        }
        case Type::SOA: {
            AUTO_R(rb, er, unpackSOAResource(msg, off));
            return {"SOA", rb, er};
        }
        case Type::PTR: {
            AUTO_R(rb, er, unpackPTRResource(msg, off));
            return {"PTR", rb, er};
        }
        case Type::MX: {
            AUTO_R(rb, er, unpackMXResource(msg, off));
            return {"MX", rb, er};
        }
        case Type::TXT: {
            AUTO_R(rb, er, unpackTXTResource(msg, off, hdr.Length));
            return {"TXT", rb, er};
        }
        case Type::AAAA: {
            AUTO_R(rb, er, unpackAAAAResource(msg, off));
            return {"AAAA", rb, er};
        }
        case Type::SRV: {
            AUTO_R(rb, er, unpackSRVResource(msg, off));
            return {"SRV", rb, er};
        }
        case Type::OPT: {
            AUTO_R(rb, er, unpackOPTResource(msg, off, hdr.Length));
            return {"OPT", rb, er};
        }
        default: {
            AUTO_R(rb, er, unpackUnknownResource(hdr.Type, msg, off, hdr.Length));
            return {"Unknown", rb, er};
        }
    }
}

R<ResourceBody, int, error> unpackResourceBody(bytez<> msg, int off, ResourceHeader hdr) {
    AUTO_R(name, r, err, _unpackResourceBody(msg, off, hdr));
    if (err != nil) {
        return {nil, off, xx::nestedError(name + " record", err)};
    }
    return {r, off + int(hdr.Length), nil};
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

////////////////////////////////////////////////////////////////////////////////

// Start parses the header and enables the parsing of Questions.
R<Header, error> Parser::Start(bytez<> msg) {
    auto& p = *this;
    p.msg = msg;
    AUTO_R(off, err, p.header.unpack(msg, 0));
    p.off = off;
    if (err != nil) {
        return {Header{}, xx::nestedError("unpacking header", err)};
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
        return {Resource{}, xx::nestedError(string("unpacking ") + xx::sectionNames(sec), er2)};
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

error Parser::skipResource(xx::section sec) {
    auto& p = *this;
    if (p.resHeaderValid) {
        int newOff = p.off + int(p.resHeader.Length);
        if (newOff > len(p.msg)) {
            return xx::errResourceLen;
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
    AUTO_R(_off, err2, xx::skipResource(p.msg, p.off));
    p.off = _off;
    if (err2 != nil) {
        return xx::nestedError(string("skipping: ") + xx::sectionNames(sec), err2);
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
        return {{}, xx::nestedError("unpacking Question.Name", err0)};
    }

    AUTO_R(typ, _off1, err1, unpackType(p.msg, off));
    off = _off1;
    ret.Type = typ;
    if (err1 != nil) {
        return {{}, xx::nestedError("unpacking Question.Type", err1)};
    }

    AUTO_R(clasz, _off2, err2, unpackClass(p.msg, off));
    off = _off2;
    ret.Class = clasz;
    if (err2 != nil) {
        return {{}, xx::nestedError("unpacking Question.Class", err2)};
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
        return xx::nestedError("skipping Question Name", err1);
    }

    AUTO_R(off2, err2, skipType(p.msg, off1));
    if (err2 != nil) {
        return xx::nestedError("skipping Question Type", err2);
    }

    AUTO_R(off3, err3, skipClass(p.msg, off2));
    if (err3 != nil) {
        return xx::nestedError("skipping Question Class", err3);
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::CNAME) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::MX) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::NS) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::PTR) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::SOA) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::TXT) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::SRV) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::A) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::AAAA) {
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
    if (!p.resHeaderValid || p.resHeader.Type != Type::OPT) {
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

}  // namespace dnsmessage
}  // namespace gx

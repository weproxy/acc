//
// weproxy@foxmail.com 2022/10/23
//

#pragma once

#include "gx/errors/errors.h"

namespace gx {
namespace dnsmessage {

// Message formats

// A Type is a type of DNS request and response.
using Type = uint16;

// ResourceHeader.Type and Question.Type
const Type TypeA = 1;
const Type TypeNS = 2;
const Type TypeCNAME = 5;
const Type TypeSOA = 6;
const Type TypePTR = 12;
const Type TypeMX = 15;
const Type TypeTXT = 16;
const Type TypeAAAA = 28;
const Type TypeSRV = 33;
const Type TypeOPT = 41;

// Question.Type
const Type TypeWKS = 11;
const Type TypeHINFO = 13;
const Type TypeMINFO = 14;
const Type TypeAXFR = 252;
const Type TypeALL = 255;

// TypeString ...
const char* TypeString(Type typ);

// A Class is a type of network.
using Class = uint16;

// ResourceHeader.Class and Question.Class
const Class ClassINET = 1;
const Class ClassCSNET = 2;
const Class ClassCHAOS = 3;
const Class ClassHESIOD = 4;

// Question.Class
const Class ClassANY = 255;

// An OpCode is a DNS operation code.
using OpCode = uint16;

// An RCode is a DNS response status code.
using RCode = uint16;

// Header.RCode values.
const RCode RCodeSuccess = 0;         // NoError
const RCode RCodeFormatError = 1;     // FormErr
const RCode RCodeServerFailure = 2;   // ServFail
const RCode RCodeNameError = 3;       // NXDomain
const RCode RCodeNotImplemented = 4;  // NotImp
const RCode RCodeRefused = 5;         // Refused

// RCodeString ...
const char* RCodeString(RCode typ);

// Header is a representation of a DNS message header.
struct Header {
    uint16 ID{0};
    bool Response{false};
    OpCode OpCode{0};
    bool Authoritative{false};
    bool Truncated{false};
    bool RecursionDesired{false};
    bool RecursionAvailable{false};
    bool AuthenticData{false};
    bool CheckingDisabled{false};
    RCode RCode{0};

    R<uint16 /*id*/, uint16 /*bits*/> pack();
};

const int nameLen = 255;

using packResult = R<bytez<> /*msg*/, error /*err*/>;

// A Name is a non-encoded domain name. It is used instead of strings to avoid
// allocations.
struct Name {
    bytez<> Data{make(nameLen)};  // 255 bytes
    uint8 Length{0};

    // pack appends the wire format of the Name to msg.
    //
    // Domain names are a sequence of counted strings split at the dots. They end
    // with a zero-length string. Compression can be used to reuse domain suffixes.
    //
    // The compression map will be updated with new domain suffixes. If compression
    // is nil, compression will not be used.
    packResult pack(bytez<> msg, map<string, int> compression, int compressionOff);

    // unpack unpacks a domain name.
    R<int, error> unpack(bytez<> msg, int off) { return unpackCompressed(msg, off, true /* allowCompression */); }

    // unpack unpacks a domain name.
    R<int, error> unpackCompressed(bytez<> msg, int off, bool allowCompression);
};

// A Question is a DNS query.
struct Question {
    Name Name;
    Type Type;
    Class Class;

    R<uint16 /*id*/, uint16 /*bits*/> pack();
};

using section = uint8;
const section sectionNotStarted = 0;
const section sectionHeader = 1;
const section sectionQuestions = 2;
const section sectionAnswers = 3;
const section sectionAuthorities = 4;
const section sectionAdditionals = 5;
const section sectionDone = 6;

// header is the wire format for a DNS message header.
struct header {
    uint16 id{0};
    uint16 bits{0};
    uint16 questions{0};
    uint16 answers{0};
    uint16 authorities{0};
    uint16 additionals{0};

    uint16 count(section sec) {
        auto& h = *this;
        switch (sec) {
            case sectionQuestions:
                return h.questions;
            case sectionAnswers:
                return h.answers;
            case sectionAuthorities:
                return h.authorities;
            case sectionAdditionals:
                return h.additionals;
        }
        return 0;
    }

    R<int, error> unpack(bytez<> msg, int off);

    Header toHeader();
};

// A ResourceBody is a DNS resource record minus the header.
struct ResourceBody {
    // pack packs a Resource except for its header.
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) = 0;

    // realType returns the actual type of the Resource. This is used to
    // fill in the header Type field.
    virtual Type realType() = 0;

    // GoString implements fmt.GoStringer.GoString.
    // virtual string GoString() = 0;
};

// A ResourceHeader is the header of a DNS resource record. There are
// many types of DNS resource records, but they all share the same header.
struct ResourceHeader {
    // Name is the domain name for which this resource record pertains.
    Name Name;

    // Type is the type of DNS resource record.
    //
    // This field will be set automatically during packing.
    Type Type;

    // Class is the class of network to which this DNS resource record
    // pertains.
    Class Class;

    // TTL is the length of time (measured in seconds) which this resource
    // record is valid for (time to live). All Resources in a set should
    // have the same TTL (RFC 2181 Section 5.2).
    uint32 TTL{0};

    // Length is the length of data in the resource record after the header.
    //
    // This field will be set automatically during packing.
    uint16 Length{0};

    R<bytez<> /*msg*/, int /*lenOff*/, error /*err*/> pack(bytez<> oldMsg, map<string, int> compression,
                                                           int compressionOff);

    R<int, error> unpack(bytez<> msg, int off);

    error fixLen(bytez<> msg, int lenOff, int preLen);
};

// A Resource is a DNS resource record.
struct Resource {
    ResourceHeader Header;
    Ref<ResourceBody> Body;
};

R<Ref<ResourceBody>, int, error> unpackResourceBody(bytez<> msg, int off, ResourceHeader hdr);

// A CNAMEResource is a CNAME Resource record.
struct CNAMEResource : public ResourceBody {
    Name CNAME;

    virtual Type realType() override { return TypeCNAME; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override {
        return CNAME.pack(msg, compression, compressionOff);
    }
};

R<Ref<CNAMEResource>, error> unpackCNAMEResource(bytez<> msg, int off);

// An MXResource is an MX Resource record.
struct MXResource : public ResourceBody {
    uint16 Pref{0};
    Name MX;

    virtual Type realType() override { return TypeMX; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override;
};

R<Ref<MXResource>, error> unpackMXResource(bytez<> msg, int off);

// An NSResource is an NS Resource record.
struct NSResource : public ResourceBody {
    Name NS;

    virtual Type realType() override { return TypeNS; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override {
        return NS.pack(msg, compression, compressionOff);
    }
};

R<Ref<NSResource>, error> unpackNSResource(bytez<> msg, int off);

// A PTRResource is a PTR Resource record.
struct PTRResource : public ResourceBody {
    Name PTR;

    virtual Type realType() override { return TypePTR; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override {
        return PTR.pack(msg, compression, compressionOff);
    }
};

R<Ref<PTRResource>, error> unpackPTRResource(bytez<> msg, int off);

// An SOAResource is an SOA Resource record.
struct SOAResource : public ResourceBody {
    Name NS;
    Name MBox;
    uint32 Serial{0};
    uint32 Refresh{0};
    uint32 Retry{0};
    uint32 Expire{0};

    // MinTTL the is the default TTL of Resources records which did not
    // contain a TTL value and the TTL of negative responses. (RFC 2308
    // Section 4)
    uint32 MinTTL{0};

    virtual Type realType() override { return TypeSOA; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override;
};

R<Ref<SOAResource>, error> unpackSOAResource(bytez<> msg, int off);

// A TXTResource is a TXT Resource record.
struct TXTResource : public ResourceBody {
    slice<string> TXT;

    virtual Type realType() override { return TypeTXT; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override;
};

R<Ref<TXTResource>, error> unpackTXTResource(bytez<> msg, int off, uint16 length);

// An SRVResource is an SRV Resource record.
struct SRVResource : public ResourceBody {
    uint16 Priority{0};
    uint16 Weight{0};
    uint16 Port{0};
    Name Target;  // Not compressed as per RFC 2782.

    virtual Type realType() override { return TypeSRV; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override;
};

R<Ref<SRVResource>, error> unpackSRVResource(bytez<> msg, int off);

// An AResource is an A Resource record.
struct AResource : public ResourceBody {
    bytez<> A{make(4)};

    virtual Type realType() override { return TypeA; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override;
};

R<Ref<AResource>, error> unpackAResource(bytez<> msg, int off);

// An AAAAResource is an AAAA Resource record.
struct AAAAResource : public ResourceBody {
    bytez<> AAAA{make(16)};

    virtual Type realType() override { return TypeAAAA; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override;
};

R<Ref<AAAAResource>, error> unpackAAAAResource(bytez<> msg, int off);

// An Option represents a DNS message option within OPTResource.
//
// The message option is part of the extension mechanisms for DNS as
// defined in RFC 6891.
struct Option {
    uint16 Code{0};  // option code
    bytez<> Data;
};

// An OPTResource is an OPT pseudo Resource record.
//
// The pseudo resource record is part of the extension mechanisms for DNS
// as defined in RFC 6891.
struct OPTResource : public ResourceBody {
    slice<Option> Options;

    virtual Type realType() override { return TypeOPT; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override;
};

R<Ref<OPTResource>, error> unpackOPTResource(bytez<> msg, int off, uint16 length);

// An UnknownResource is a catch-all container for unknown record types.
struct UnknownResource : public ResourceBody {
    Type Type_;
    bytez<> Data;

    virtual Type realType() override { return Type_; }
    virtual packResult pack(bytez<> msg, map<string, int> compression, int compressionOff) override;
};

R<Ref<UnknownResource>, error> unpackUnknownResource(Type recordType, bytez<> msg, int off, uint16 length);

// Message is a representation of a DNS message.
struct Message {
    Header Header;
    slice<Question> Questions;
    slice<Resource> Answers;
    slice<Resource> Authorities;
    slice<Resource> Additionals;

    // Unpack parses a full Message.
    error Unpack(bytez<> msg);

    // Pack packs a full Message.
    R<bytez<>, error> Pack();

    // AppendPack is like Pack but appends the full Message to b and returns the
    // extended buffer.
    R<bytez<>, error> AppendPack(bytez<> b);
};

// A Parser allows incrementally parsing a DNS message.
//
// When parsing is started, the Header is parsed. Next, each Question can be
// either parsed or skipped. Alternatively, all Questions can be skipped at
// once. When all Questions have been parsed, attempting to parse Questions
// will return (nil, nil) and attempting to skip Questions will return
// (true, nil). After all Questions have been either parsed or skipped, all
// Answers, Authorities and Additionals can be either parsed or skipped in the
// same way, and each type of Resource must be fully parsed or skipped before
// proceeding to the next type of Resource.
//
// Note that there is no requirement to fully skip or parse the message.
struct Parser {
    bytez<> msg;
    header header;

    section section_;
    int off{0};
    int index{0};
    bool resHeaderValid{false};
    ResourceHeader resHeader;

    // Start parses the header and enables the parsing of Questions.
    R<Header, error> Start(bytez<> msg);

    error checkAdvance(section sec );

    R<Resource, error> resource(section sec ) ;

    R<ResourceHeader, error> resourceHeader(section sec ) ;

    error skipResource(section sec ) ;
};

}  // namespace dnsmessage
}  // namespace gx

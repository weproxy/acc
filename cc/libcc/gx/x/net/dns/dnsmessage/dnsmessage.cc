//
// weproxy@foxmail.com 2022/10/23
//

#include "dnsmessage.h"

namespace gx {
namespace dnsmessage {

// ErrNotStarted indicates that the prerequisite information isn't
// available yet because the previous records haven't been appropriately
// parsed, skipped or finished.
const error ErrNotStarted = errors::New("parsing/packing of this type isn't available yet");

// ErrSectionDone indicated that all records in the section have been
// parsed or finished.
const error ErrSectionDone = errors::New("parsing/packing of this section has completed");

namespace xx {
const error errBaseLen = errors::New("insufficient data for base length type");
const error errCalcLen = errors::New("insufficient data for calculated length type");
const error errReserved = errors::New("segment prefix is reserved");
const error errTooManyPtr = errors::New("too many pointers (>10)");
const error errInvalidPtr = errors::New("invalid pointer");
const error errNilResouceBody = errors::New("nil resource body");
const error errResourceLen = errors::New("insufficient data for resource body length");
const error errSegTooLong = errors::New("segment length too long");
const error errZeroSegLen = errors::New("zero length segment");
const error errResTooLong = errors::New("resource length too long");
const error errTooManyQuestions = errors::New("too many Questions to pack (>65535)");
const error errTooManyAnswers = errors::New("too many Answers to pack (>65535)");
const error errTooManyAuthorities = errors::New("too many Authorities to pack (>65535)");
const error errTooManyAdditionals = errors::New("too many Additionals to pack (>65535)");
const error errNonCanonicalName = errors::New("name is not in canonical format (it must end with a .)");
const error errStringTooLong = errors::New("character string exceeds maximum length (255)");
const error errCompressedSRV = errors::New("compressed name in SRV resource data");

error nestedError(const string& s, const error err) { return errors::New(s + ": " + err->String()); }
}  // namespace xx

// ToString ...
const char* ToString(const Type e) {
#define CASE_RETURN_TYPE(e) \
    case Type::e:           \
        return "Type" #e

    switch (e) {
        CASE_RETURN_TYPE(A);
        CASE_RETURN_TYPE(NS);
        CASE_RETURN_TYPE(CNAME);
        CASE_RETURN_TYPE(SOA);
        CASE_RETURN_TYPE(PTR);
        CASE_RETURN_TYPE(MX);
        CASE_RETURN_TYPE(TXT);
        CASE_RETURN_TYPE(AAAA);
        CASE_RETURN_TYPE(SRV);
        CASE_RETURN_TYPE(OPT);
        CASE_RETURN_TYPE(WKS);
        CASE_RETURN_TYPE(HINFO);
        CASE_RETURN_TYPE(MINFO);
        CASE_RETURN_TYPE(AXFR);
        CASE_RETURN_TYPE(ALL);
        default:
            return "";
    }
}

// ToString ...
const char* ToString(const Class e) {
#define CASE_RETURN_CLASS(e) \
    case Class::e:           \
        return "Class" #e

    switch (e) {
        CASE_RETURN_CLASS(INET);
        CASE_RETURN_CLASS(CSNET);
        CASE_RETURN_CLASS(CHAOS);
        CASE_RETURN_CLASS(HESIOD);
        CASE_RETURN_CLASS(ANY);
        default:
            return "";
    }
}

// ToString ...
const char* ToString(const RCode e) {
#define CASE_RETURN_RCODE(e) \
    case RCode::e:           \
        return "RCode" #e

    switch (e) {
        CASE_RETURN_RCODE(Success);
        CASE_RETURN_RCODE(FormatError);
        CASE_RETURN_RCODE(ServerFailure);
        CASE_RETURN_RCODE(NameError);
        CASE_RETURN_RCODE(NotImplemented);
        CASE_RETURN_RCODE(Refused);
        default:
            return "";
    }
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

}  // namespace dnsmessage
}  // namespace gx

//
// weproxy@foxmail.com 2022/10/23
//

#include "dns.h"

namespace nx {
namespace dns {

// ...
const error ErrQueryNotFound = errors::New("not found");
const error ErrQueryDropped = errors::New("dropped");
const error ErrQueryFaked = errors::New("faked");
const error ErrQueryHited = errors::New("hited");

// toAnswerString ...
string toAnswerString(slice<dnsmessage::Resource> arr) {
    slice<string> answs;

    // // for _, answ := range arr {
    // for (int i = 0; i < len(arr); i++) {
    //     auto answ = arr[i];
    // 	if (answ->Body == nil) {
    // 		continue;
    // 	}
    // 	switch (ans->Header.Type) {
    // 	case dnsmessage::TypeA:
    // 		answs = append(answs, fmt.Sprintf("%v", net.IP(answ.Body.(*dnsmessage.AResource).A[:])))
    //         break;
    // 	case dnsmessage.TypeAAAA:
    // 		answs = append(answs, fmt.Sprintf("%v", net.IP(answ.Body.(*dnsmessage.AAAAResource).AAAA[:])))
    //         break;
    // 	case dnsmessage.TypeCNAME:
    // 		answs = append(answs, strings.TrimRight(answ.Body.(*dnsmessage.CNAMEResource).CNAME.String(), "."))
    //         break;
    // 	// case dnsmessage.TypeNS:
    // 	// 	answs = append(answs, answ.Body.(*dnsmessage.NSResource).NS.String()+"NS")
    // 	default:
    // 		answs = append(answs, fmt.Sprintf("Type%v", int(answ.Header.Type)))
    //         break;
    // 	}
    // }

    // return GX_SS(answs);
    return "";
}

}  // namespace dns
}  // namespace nx

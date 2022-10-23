//
// weproxy@foxmail.com 2022/10/23
//

#include "dns.h"
#include "gx/strings/strings.h"
#include "gx/sync/sync.h"
#include "gx/time/time.h"
#include "logx/logx.h"

namespace nx {
namespace dns {

// makeCacheKey ...
string makeCacheKey(const Question& q) { return string(q.Name.String() + dnsmessage::TypeString(q.Type)); }

// cacheEntry ...
struct cacheEntry {
    Ref<Message> msg;
    time::Time exp;

    cacheEntry() = default;
    cacheEntry(Ref<Message> m, time::Time e) : msg(m), exp(e) {}
};

// cacheMap ...
struct cacheMap {
    sync::Mutex mu;
    map<string, Ref<cacheEntry>> mem;

    // load
    R<Ref<Answer>, error> load(Ref<Message> msg);

    // store ...
    error store(Ref<Message> msg);

    // Clear ...
    void Clear() {
        mu.Lock();
        mem = makemap<string, Ref<cacheEntry>>();
        mu.Unlock();
    }

    // cleanUp() ...
    void cleanUp();
};

// newCacheMap ...
Ref<cacheMap> newCacheMap() {
    auto cache = NewRef<cacheMap>();
    gx::go([cache] { cache->cleanUp(); });
    return cache;
}

extern string toAnswerString(slice<dnsmessage::Resource> arr);

// load ...
R<Ref<Answer>, error> cacheMap::load(Ref<Message> msg) {
    if (!msg || len(msg->Questions) == 0) {
        return {nil, nil};
    }

    auto& m = *this;

    LOGS_I("[dns] cache.Query " << msg->Questions[0].Name << dnsmessage::TypeString(msg->Questions[0].Type));

    if (true) {
        // LOGS_W("[dns] cache.Answer " << msg->Questions[0].Name << dnsmessage::TypeString(msg->Questions[0].Type));
        return {nil, nil};
    }

    auto key = makeCacheKey(msg->Questions[0]);

    m.mu.Lock();
    DEFER(m.mu.Unlock());

    AUTO_R(entry, ok, m.mem(key));
    if (!ok) {
        return {nil, nil};
    }
    if (time::Now().After(entry->exp)) {
        delmap(m.mem, key);
        return {nil, nil};
    }

    auto amsg = NewRef<Message>();
    amsg->Answers = entry->msg->Answers;
    amsg->Additionals = entry->msg->Additionals;
    amsg->Authorities = entry->msg->Authorities;

    amsg->Header.ID = msg->Header.ID;
    amsg->Header.Response = true;
    amsg->Header.RCode = entry->msg->Header.RCode;
    amsg->Header.OpCode = entry->msg->Header.OpCode;
    amsg->Header.Authoritative = entry->msg->Header.Authoritative;
    amsg->Header.Truncated = entry->msg->Header.Truncated;
    amsg->Header.RecursionDesired = entry->msg->Header.RecursionDesired;
    amsg->Header.RecursionAvailable = entry->msg->Header.RecursionAvailable;

    AUTO_R(data, err, amsg->Pack());
    if (err != nil) {
        return {nil, err};
    }

    auto answer = NewRef<Answer>();
    answer->Msg = amsg;
    answer->Name = strings::TrimSuffix(msg->Questions[0].Name.String(), ".");
    answer->Data = data;

    LOGS_W("[dns] cache.Answer " << msg->Questions[0].Name << dnsmessage::TypeString(msg->Questions[0].Type) << " <- "
                                 << toAnswerString(answer->Msg->Answers));

    return {answer, nil};
}

// store ...
error cacheMap::store(Ref<Message> msg) {
    if (msg == nil || msg->Header.RCode != dnsmessage::RCodeSuccess || len(msg->Questions) == 0) {
        return nil;
    }

#if 0
    auto key = makeCacheKey(msg->Questions[0]);
    auto exp = time::Now().Add(time::Second * 300);

    auto& m = *this;

    m.mu.Lock();
    m.mem[key] = NewRef<cacheEntry>(msg, exp);
    m.mu.Unlock();
#endif

    LOGS_W("[dns] cache.Store " << msg->Questions[0].Name << dnsmessage::TypeString(msg->Questions[0].Type) << " <- "
                                 << toAnswerString(msg->Answers));

    return nil;
}

// cleanUp ...
void cacheMap::cleanUp() {
    auto dura = 30 * time::Second;
    // if runtime.GOOS == "darwin" {
    // 	dura = 10 * time.Second
    // }

    auto cleanFn = [] {
        // var delKeys []string
        // for key, entry := range m.mem {
        // 	if !time.Now().Before(entry->exp) {
        // 		delKeys = append(delKeys, key)
        // 	}
        // }
        // if len(delKeys) > 0 {
        // 	for _, key := range delKeys {
        // 		delete(m.mem, key)
        // 	}
        // 	logx.W("[dns] cache.cleanUp %v, remain %v entries", len(delKeys), len(m.mem))
        // }
    };

    // t := time.NewTicker(dura)
    // defer t.Stop()

    // for range t.C {
    // 	m.mu.Lock()
    // 	if len(m.mem) > 0 {
    // 		cleanFn()
    // 	}
    // 	m.mu.Unlock()
    // }
}

////////////////////////////////////////////////////////////////////////////////

static Ref<cacheMap> _cache;

// OnRequest ...
R<Ref<Message>, Ref<Answer>, error> OnRequest(bytez<> b) {
    auto msg = NewRef<Message>();
    auto err = msg->Unpack(b);
    if (err) {
        LOGS_E("[dns] err: " << err);
        return {nil, nil, err};
    }

    // LOGS_I("[dns] cache.Query: ");
    AUTO_R(answ, _, _cache->load(msg));

    return {msg, answ, nil};
}

// OnResponse ...
R<Ref<Message>, error> OnResponse(bytez<> b) {
    auto msg = NewRef<Message>();
    auto err = msg->Unpack(b);
    if (err) {
        LOGS_E("[dns] err: " << err);
        return {nil, err};
    }

    // if (len(msg->Questions) > 0) {
    //     LOGS_W("[dns] cache.Store: " << msg->Questions[0].Name);
    // }
    err = _cache->store(msg);

    return {msg, nil};
}

}  // namespace dns
}  // namespace nx

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
string makeCacheKey(const Question& q) { return q.Name.String() + ToString(q.Type); }

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
    map<string, Ref<cacheEntry>> mem{makemap<string, Ref<cacheEntry>>()};

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

extern string toAnswerString(slice<Resource> arr);

// load ...
R<Ref<Answer>, error> cacheMap::load(Ref<Message> msg) {
    if (!msg || len(msg->Questions) == 0) {
        return {nil, nil};
    }

    auto& m = *this;
    auto& q = msg->Questions[0];

    LOGS_I("[dns] cache.Query " << q.Name << q.Type);

    // check if fake provide
    AUTO_R(answer, err, MakeFakeAnswer(msg.get(), nil));
    if (!err && answer && answer->Msg) {
        LOGS_W("[dns] cache.FakeAnswer " << q.Name << q.Type << " <- " << answer->Msg->Answers);
        return {answer, nil};
    }

    auto key = makeCacheKey(q);

    m.mu.Lock();
    DEFER(m.mu.Unlock());

    AUTO_R(entry, ok, m.mem(key));
    if (!ok || !entry || !entry->msg) {
        return {nil, nil};
    }
    if (time::Now().After(entry->exp)) {
        delmap(m.mem, key);
        return {nil, nil};
    }

    auto amsg = NewRef<Message>();
    *amsg = *entry->msg;
    amsg->Header.ID = msg->Header.ID;
    amsg->Header.Response = true;

    answer = NewRef<Answer>();
    answer->Msg = amsg;

    LOGS_W("[dns] cache.Answer " << q.Name << q.Type << " <- " << amsg->Answers);

    return {answer, nil};
}

// store ...
error cacheMap::store(Ref<Message> msg) {
    if (msg == nil || msg->Header.RCode != RCode::Success || len(msg->Questions) == 0) {
        return nil;
    }

    auto& m = *this;
    auto& q = msg->Questions[0];

    auto key = makeCacheKey(q);
    auto exp = time::Now().Add(time::Second * 300);

    m.mu.Lock();
    m.mem[key] = NewRef<cacheEntry>(msg, exp);
    m.mu.Unlock();

    LOGS_W("[dns] cache.Store " << q.Name << q.Type << " <- " << msg->Answers);

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

static Ref<cacheMap> _cache(NewRef<cacheMap>());

// cacheLoad ...
R<Ref<Answer>, error> cacheLoad(Ref<Message> msg) { return _cache->load(msg); }

// cacheStore ...
error cacheStore(Ref<Message> msg) { return _cache->store(msg); }

}  // namespace dns
}  // namespace nx

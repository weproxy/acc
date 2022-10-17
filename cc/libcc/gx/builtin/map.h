//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
// map ...
template <typename K, typename V>
struct map {
    MapPtr<K, V> p_;

    // map ...
    map() : p_(new Map<K, V>()) {}

    Map<K, V>& operator*() { return *p_; }
    const Map<K, V>& operator*() const { return *p_; }

    MapPtr<K, V> operator->() { return p_; }
    const MapPtr<K, V> operator->() const { return p_; }

    // [k]
    V& operator[](const K& k) { return p_->operator[](k); }
    const V& operator[](const K& k) const { return p_->operator[](k); }

    R<V, bool> operator()(const K& k) const {
        if (p_) {
            auto it = p_->find(k);
            if (it != p_->end()) {
                return {it->second, true};
            }
        }
        return {{}, false};
    }

    void del(const K& k) const {
        if (p_) {
            p_->erase(k);
        }
    }

    int size() const { return p_ ? p_->size() : 0; }

    // bool() ...
    operator bool() const { return !!p_; }
};

// makemap ...
template <typename K, typename V>
map<K, V> makemap() {
    return map<K, V>();
}

template <typename K, typename V>
void delmap(map<K, V> m, const K& k) {
    m.del(k);
}

// len ...
template <typename K, typename V>
int len(const map<K, V>& m) {
    return m.size();
}

}  // namespace gx

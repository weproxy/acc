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
    MapPtr<K, V> map_;

    // [k]
    V& operator[](const K& k) { return ptr->operator[](k); }
    const V& operator[](const K& k) const { return ptr->operator[](k); }

    // bool() ...
    operator bool() const { return !!map_; }
};

// makemap ...
template <typename K, typename V>
map<K, V> makemap() {
    return map<K, V>();
}

}  // namespace gx

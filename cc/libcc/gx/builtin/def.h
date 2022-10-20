//
// weproxy@foxmail.com 2022/10/03
//

#pragma once

#include <unistd.h>

#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//
namespace gx {

// Ref ...
template <typename T>
using Ref = std::shared_ptr<T>;

// Box ...
template <typename T>
using Box = std::unique_ptr<T>;

// Weak ...
template <typename T>
using Weak = std::weak_ptr<T>;

// func ...
template <typename T>
using func = std::function<T>;

// Vec ...
template <typename T>
using Vec = std::vector<T>;

// VecRef ...
template <typename T>
using VecRef = Ref<Vec<T>>;

// VecBox ...
template <typename T>
using VecBox = Box<Vec<T>>;

// Set ...
template <typename T>
using Set = std::set<T>;

// SetRef ...
template <typename T>
using SetRef = Ref<Set<T>>;

// SetBox ...
template <typename T>
using SetBox = Box<Set<T>>;

// HashSet ...
template <typename T>
using HashSet = std::unordered_set<T>;

// MultiSet ...
template <typename T>
using MultiSet = std::multiset<T>;

// List ...
template <typename T>
using List = std::list<T>;

// ListRef ...
template <typename T>
using ListRef = Ref<List<T>>;

// ListRef ...
template <typename T>
using ListBox = Box<List<T>>;

// Map ...
template <typename K, typename V>
using Map = std::map<K, V>;

// MapRef ...
template <typename K, typename V>
using MapRef = Ref<Map<K, V>>;

// MapBox ...
template <typename K, typename V>
using MapBox = Box<Map<K, V>>;

// HashMap ...
template <typename K, typename V>
using HashMap = std::unordered_map<K, V>;

// Deque ...
template <typename T>
using Deque = std::deque<T>;

}  // namespace gx

////////////////////////////////////////////////////////////////////////////////
#if __cplusplus < 201402L  // < C++14
namespace std {
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}  // namespace std
#endif

namespace gx {
// NewRef ...
template <typename T, typename... Args>
Ref<T> NewRef(Args&&... args) {
    return std::make_shared<T, Args...>(std::forward<Args>(args)...);
}

// NewBox ...
template <typename T, typename... Args>
Box<T> NewBox(Args&&... args) {
    return std::make_unique<T, Args...>(std::forward<Args>(args)...);
}
}  // namespace gx

////////////////////////////////////////////////////////////////////////////////
//
namespace gx {
// string ...
using string = std::string;

// strvec ...
using strvec = Vec<string>;

// err_t ...
struct err_t {
    virtual ~err_t() {}
    virtual string String() const = 0;
    virtual string Error() const { return String(); }
    operator string() const { return String(); }
};

// error ...
using error = Ref<err_t>;

// nil ...
#define nil nullptr

// strerr_t ...
struct strerr_t : public err_t {
    strerr_t(const string& s) : s_(s) {}
    virtual string String() const { return s_; };
    string s_;
    static error New(const string& s) { return error(new strerr_t(s)); }
};

}  // namespace gx

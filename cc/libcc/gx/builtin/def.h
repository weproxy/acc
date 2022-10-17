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

////////////////////////////////////////////////////////////////////////////////////////////////////
//
namespace gx {

// SharedPtr ...
template <typename T>
using SharedPtr = std::shared_ptr<T>;

// UniquePtr ...
template <typename T>
using UniquePtr = std::unique_ptr<T>;

// WeakPtr ...
template <typename T>
using WeakPtr = std::weak_ptr<T>;

// Vec ...
template <typename T>
using Vec = std::vector<T>;

// VecPtr ...
template <typename T>
using VecPtr = SharedPtr<Vec<T>>;

// Set ...
template <typename T>
using Set = std::set<T>;

// SetPtr ...
template <typename T>
using SetPtr = SharedPtr<Set<T>>;

// HashSet ...
template <typename T>
using HashSet = std::unordered_set<T>;

// MultiSet ...
template <typename T>
using MultiSet = std::multiset<T>;

// List ...
template <typename T>
using List = std::list<T>;

// ListPtr ...
template <typename T>
using ListPtr = SharedPtr<List<T>>;

// Map ...
template <typename K, typename V>
using Map = std::map<K, V>;

// MapPtr ...
template <typename K, typename V>
using MapPtr = SharedPtr<Map<K, V>>;

// HashMap ...
template <typename K, typename V>
using HashMap = std::unordered_map<K, V>;

// Deque ...
template <typename T>
using Deque = std::deque<T>;

}  // namespace gx

////////////////////////////////////////////////////////////////////////////////////////////////////
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
using error = SharedPtr<err_t>;

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

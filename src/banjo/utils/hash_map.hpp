#ifndef BANJO_UTILS_HASH_MAP_H
#define BANJO_UTILS_HASH_MAP_H

#include <initializer_list>
#include <unordered_map>
#include <utility>

namespace banjo {

template <typename Key, typename Value>
class HashMap {

private:
    typedef std::unordered_map<Key, Value>::value_type ImplValueType;

    std::unordered_map<Key, Value> impl;

public:
    HashMap() {}
    HashMap(std::initializer_list<ImplValueType> values) : impl{values} {}

    void insert(Key &&key, Value &&value) { impl.emplace(key, value); }
    void insert(Key &key, Value &value) { impl.emplace(std::move(key), std::move(value)); }

    Value *try_find(const Key &key) {
        auto iter = impl.find(key);
        return iter == impl.end() ? nullptr : &iter->second;
    }

    const Value *try_find(const Key &key) const {
        auto iter = impl.find(key);
        return iter == impl.end() ? nullptr : &iter->second;
    }
};

} // namespace banjo

#endif

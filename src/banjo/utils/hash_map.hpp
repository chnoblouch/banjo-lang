#ifndef BANJO_UTILS_HASH_MAP_H
#define BANJO_UTILS_HASH_MAP_H

#include <unordered_map>
#include <utility>

namespace banjo {

template <typename Key, typename Value>
class HashMap {

private:
    std::unordered_map<Key, Value> impl;

public:
    void insert(Key &&key, Value &&value) { impl.emplace(key, value); }
    void insert(Key &key, Value &value) { impl.emplace(std::move(key), std::move(value)); }

    Value *try_find(const Key &key) {
        auto iter = impl.find(key);
        return iter == impl.end() ? nullptr : &iter->second;
    }
};

} // namespace banjo

#endif

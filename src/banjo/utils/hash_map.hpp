#ifndef BANJO_UTILS_HASH_MAP_H
#define BANJO_UTILS_HASH_MAP_H

#include <initializer_list>
#include <unordered_map>
#include <utility>

namespace banjo {

template <typename Key, typename Value>
class HashMap {

private:
    typedef std::unordered_map<Key, Value> ImplType;
    typedef ImplType::value_type ImplValueType;

    ImplType impl;

public:
    HashMap() {}
    HashMap(std::initializer_list<ImplValueType> values) : impl{values} {}

    void insert(Key &&key, Value &&value) { impl.emplace(key, value); }
    void insert(Key key, Value value) { impl.emplace(std::move(key), std::move(value)); }

    void overwrite(Key &&key, Value &&value) { impl[key] = value; }
    void overwrite(Key key, Value value) { impl[std::move(key)] = std::move(value); }

    void clear() { impl.clear(); }

    Value *try_find(const Key &key) {
        auto iter = impl.find(key);
        return iter == impl.end() ? nullptr : &iter->second;
    }

    const Value *try_find(const Key &key) const {
        auto iter = impl.find(key);
        return iter == impl.end() ? nullptr : &iter->second;
    }

    Value &find(const Key &key) { return *try_find(key); }
    const Value &find(const Key &key) const { return *try_find(key); }

    Value &find_or_create(const Key &key) { return impl[key]; }
    const Value &find_or_create(const Key &key) const { return impl[key]; }

    ImplType::iterator begin() { return impl.begin(); }
    ImplType::iterator end() { return impl.end(); }

    ImplType::const_iterator begin() const { return impl.begin(); }
    ImplType::const_iterator end() const { return impl.end(); }
};

} // namespace banjo

#endif

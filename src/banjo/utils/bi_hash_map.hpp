#ifndef BANJO_UTILS_BI_HASH_MAP_H
#define BANJO_UTILS_BI_HASH_MAP_H

#include "banjo/utils/hash_map.hpp"

#include <initializer_list>
#include <utility>

namespace banjo {

template <typename Left, typename Right>
class BiHashMap {

private:
    typedef HashMap<Left, Right> MapLTR;
    typedef HashMap<Right, Left> MapRTL;

    MapLTR left_to_right;
    MapRTL right_to_left;

public:
    BiHashMap(std::initializer_list<std::pair<Left, Right>> entries) {
        for (const auto [left, right] : std::move(entries)) {
            insert(left, right);
        }
    }

    void insert(Left left, Right right) {
        left_to_right.insert(left, right);
        right_to_left.insert(right, left);
    }

    void insert(Left &&left, Right &&right) {
        left_to_right.insert(left, right);
        right_to_left.insert(right, left);
    }

    Right &find_by_left(const Left &key) { return left_to_right.find(key); }
    const Right &find_by_left(const Left &key) const { return left_to_right.find(key); }
    Left &find_by_right(const Right &key) { return right_to_left.find(key); }
    const Left &find_by_right(const Right &key) const { return right_to_left.find(key); }

    Right *try_find_by_left(const Left &key) { return left_to_right.try_find(key); }
    const Right *try_find_by_left(const Left &key) const { return left_to_right.try_find(key); }
    Left *try_find_by_right(const Right &key) { return right_to_left.try_find(key); }
    const Left *try_find_by_right(const Right &key) const { return right_to_left.try_find(key); }
};
} // namespace banjo

#endif

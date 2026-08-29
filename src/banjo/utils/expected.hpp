#ifndef BANJO_UTILS_EXPECTED_H
#define BANJO_UTILS_EXPECTED_H

#include "banjo/utils/macros.hpp"

#include <variant>

namespace banjo {

template <typename T, typename E>
class Expected {

private:
    std::variant<T, E> raw_value;

public:
    Expected(const T &value) : raw_value{value} {}
    Expected(T &&value) : raw_value{value} {}
    Expected(const E &value) : raw_value{value} {}
    Expected(E &&value) : raw_value{value} {}

    T value() {
        ASSERT(raw_value.index() == 0);
        return std::get<0>(std::move(raw_value));
    }

    E error() {
        ASSERT(raw_value.index() == 1);
        return std::get<1>(std::move(raw_value));
    }

    operator bool() const { return raw_value.index() == 0; }
};

} // namespace banjo

#endif

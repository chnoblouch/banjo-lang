#ifndef UTILS_H
#define UTILS_H

#include <initializer_list>

namespace banjo {

namespace Utils {

int align(int value, int boundary);

template <typename T>
bool is_one_of(T value, std::initializer_list<T> candidates) {
    for (T candidate : candidates) {
        if (value == candidate) {
            return true;
        }
    }

    return false;
}

} // namespace Utils

} // namespace banjo

#endif

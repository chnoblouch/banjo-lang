#ifndef BANJO_UTILS_H
#define BANJO_UTILS_H

#include "banjo/utils/fixed_vector.hpp"
#include "banjo/utils/large_int.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <vector>

namespace banjo::utils {

typedef FixedVector<unsigned char, 16> LEB128Buffer;

template <typename T>
T div_ceil(T divident, T divisor) {
    return (divident + divisor - 1) / divisor;
}

template <typename T>
std::uint32_t get_bits_32(T value) {
    static_assert(sizeof(T) == 4, "type size is not 32 bits");

    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(std::uint32_t));
    return bits;
}

template <typename T>
std::uint64_t get_bits_64(T value) {
    static_assert(sizeof(T) == 8, "type size is not 64 bits");

    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof(std::uint64_t));
    return bits;
}

template <typename T>
bool is_power_of_two(T value) {
    return (value & (value - 1)) == 0;
}

template <typename T>
unsigned first_bit_set(T value) {
    int shift = -1;

    while (value != 0) {
        value = value >> 1;
        shift += 1;
    }

    return static_cast<unsigned>(shift);
}

template <typename T, typename B>
T align(T value, B boundary) {
    if (boundary == 0) {
        return value;
    }

    T mod = value % static_cast<T>(boundary);
    return mod == 0 ? value : value + boundary - mod;
}

template <typename T, typename C>
bool is_one_of(T value, std::initializer_list<C> candidates) {
    for (C candidate : candidates) {
        if (value == candidate) {
            return true;
        }
    }

    return false;
}

template <typename A, typename B>
bool equal(const A &lhs, const B &rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (unsigned i = 0; i < lhs.size(); i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }

    return true;
}

template <typename Iterable, typename T>
bool contains(const Iterable &iterable, const T &value) {
    return std::find(iterable.begin(), iterable.end(), value) != iterable.end();
}

template <typename T>
void extend(std::vector<T> &dst, const std::vector<T> &src) {
    dst.insert(dst.end(), src.begin(), src.end());
}

template <typename T>
std::vector<T> remove_duplicates(const std::vector<T> &array) {
    std::vector<T> result;

    for (const T &element : array) {
        bool is_duplicate = false;

        for (const T &other : result) {
            if (other == element) {
                is_duplicate = true;
                break;
            }
        }

        if (!is_duplicate) {
            result.push_back(element);
        }
    }

    return result;
}

std::optional<std::uint64_t> parse_u64(std::string_view string);
std::vector<std::string_view> split_string(std::string_view string, char delimiter);
std::string convert_eol_to_lf(std::string_view string);

LEB128Buffer encode_uleb128(std::uint64_t value);
LEB128Buffer encode_sleb128(LargeInt value);

std::optional<std::string> read_string_file(const std::filesystem::path &path);
bool write_string_file(std::string_view contents, const std::filesystem::path &path);
std::optional<std::string_view> get_env(const std::string &name);

} // namespace banjo::utils

#endif

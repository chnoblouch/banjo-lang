#ifndef BANJO_UTILS_ARENA_H
#define BANJO_UTILS_ARENA_H

#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

#include <initializer_list>
#include <list>
#include <memory>
#include <span>
#include <type_traits>

namespace banjo::utils {

class Arena {

private:
    unsigned min_block_size;

    std::list<std::unique_ptr<char[]>> blocks;
    unsigned cur_block_size;
    unsigned cur_offset;

public:
    Arena(unsigned min_block_size = 2048) : min_block_size(min_block_size), cur_offset(0) {}
    Arena(const Arena &) = delete;
    Arena(Arena &&) = default;

    Arena &operator=(const Arena &) = delete;
    Arena &operator=(Arena &&) = default;

    template <typename T>
    std::span<T> allocate_array(unsigned length) {
        static_assert(std::is_trivially_destructible<T>(), "T has to be trivially destructible");
        ASSERT_MESSAGE(sizeof(T) < min_block_size, "size of T has to be less than block size");

        cur_offset = Utils::align(cur_offset, static_cast<unsigned>(alignof(T)));

        unsigned total_size = length * sizeof(T);

        if (blocks.empty() || cur_offset + total_size > cur_block_size) {
            cur_block_size = std::max(min_block_size, total_size);
            blocks.push_back(std::make_unique<char[]>(cur_block_size));
            cur_offset = 0;
        }

        std::unique_ptr<char[]> &cur_block = blocks.back();
        char *pointer = &cur_block[cur_offset];
        cur_offset += total_size;

        return std::span<T>{reinterpret_cast<T *>(pointer), length};
    }

    template <typename T>
    T *create(T &&value) {
        T *pointer = allocate_array<T>(1).data();
        return new (pointer) T(value);
    }

    template <typename T, typename... ConstructorArgs>
    T *create(ConstructorArgs... constructor_args) {
        T *pointer = allocate_array<T>(1).data();
        return new (pointer) T(constructor_args...);
    }

    template <typename T>
    std::span<T> create_array(std::span<T> values) {
        std::span<T> copy = allocate_array<T>(values.size());

        for (unsigned i = 0; i < values.size(); i++) {
            copy[i] = std::move(values[i]);
        }

        return copy;
    }

    template <typename T>
    std::span<T> create_array(std::initializer_list<T> values) {
        std::span<T> copy = allocate_array<T>(values.size());

        for (unsigned i = 0; i < values.size(); i++) {
            copy[i] = std::move(values.begin()[i]);
        }

        return copy;
    }
};

} // namespace banjo::utils

#endif

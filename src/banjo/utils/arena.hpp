#ifndef BANJO_UTILS_ARENA_H
#define BANJO_UTILS_ARENA_H

#include "banjo/utils/utils.hpp"

#include <list>
#include <memory>
#include <type_traits>

namespace banjo::utils {

template <unsigned BLOCK_SIZE>
class Arena {

public:
    std::list<std::unique_ptr<std::uint8_t[]>> blocks;
    unsigned cur_offset;

public:
    Arena() : cur_offset(0) {}
    Arena(const Arena &) = delete;
    Arena(Arena &&) = default;

    Arena &operator=(const Arena &) = delete;
    Arena &operator=(Arena &&) = default;

    template <typename T, typename... ConstructorArgs>
    T *create(ConstructorArgs... constructor_args) {
        static_assert(sizeof(T) < BLOCK_SIZE, "size of T has to be less than block size");
        static_assert(std::is_trivially_destructible<T>(), "T has to be trivially destructible");

        cur_offset = Utils::align(cur_offset, static_cast<unsigned>(alignof(T)));

        if (blocks.empty() || cur_offset + sizeof(T) > BLOCK_SIZE) {
            blocks.push_back(std::make_unique<std::uint8_t[]>(BLOCK_SIZE));
            cur_offset = 0;
        }

        std::unique_ptr<std::uint8_t[]> &cur_block = blocks.back();
        std::uint8_t *pointer = &cur_block[cur_offset];
        cur_offset += sizeof(T);

        return new (pointer) T(constructor_args...);
    }
};

} // namespace banjo::utils

#endif

#ifndef BANJO_UTILS_TYPED_ARENA_H
#define BANJO_UTILS_TYPED_ARENA_H

#include <utility>

namespace banjo::utils {

template <typename T, unsigned BLOCK_LENGTH = 64>
class TypedArena {

public:
    struct Block {
        Block *prev;
        T data[BLOCK_LENGTH];
    };

    Block *cur_block;
    unsigned cur_index;

public:
    TypedArena() : cur_block(nullptr), cur_index(0) {}

    TypedArena(const TypedArena &) = delete;

    TypedArena(TypedArena &&other) noexcept
      : cur_block(std::exchange(other.cur_block, nullptr)),
        cur_index(other.cur_index) {}

    ~TypedArena() {
        Block *block = cur_block;

        while (block) {
            for (unsigned i = 0; i < (block == cur_block ? cur_index : BLOCK_LENGTH); i++) {
                block->data[i].~T();
            }

            Block *prev_block = block->prev;
            ::operator delete(block);
            block = prev_block;
        }
    }

    TypedArena &operator=(const TypedArena &) = delete;

    TypedArena &operator=(TypedArena &&other) noexcept {
        std::swap(cur_block, other.cur_block);
        cur_index = other.cur_index;
        return *this;
    }

    template <typename... ConstructorArgs>
    T *create(ConstructorArgs... constructor_args) {
        if (!cur_block) {
            cur_block = reinterpret_cast<Block *>(::operator new(sizeof(Block)));
            cur_block->prev = nullptr;
        } else if (cur_index == BLOCK_LENGTH) {
            Block *prev_block = cur_block;
            cur_block = reinterpret_cast<Block *>(::operator new(sizeof(Block)));
            cur_block->prev = prev_block;
            cur_index = 0;
        }

        T *pointer = &cur_block->data[cur_index++];
        return new (pointer) T(constructor_args...);
    }
};

} // namespace banjo::utils

#endif

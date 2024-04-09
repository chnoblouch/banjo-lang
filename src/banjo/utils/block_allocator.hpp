#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include "utils/utils.hpp"

#include <cstdlib>
#include <utility>

template <unsigned BLOCK_SIZE>
class BlockAllocator {

private:
    struct Block {
        unsigned char *data = static_cast<unsigned char *>(std::malloc(BLOCK_SIZE));
        unsigned offset = 0;
        Block *next = nullptr;
    };

    Block *first;
    Block *last;

public:
    BlockAllocator() {
        first = new Block();
        last = first;
    }

    BlockAllocator(const BlockAllocator &) = delete;

    BlockAllocator(BlockAllocator &&other) noexcept
      : first(std::exchange(other.first, nullptr)),
        last(std::exchange(other.last, nullptr)) {}

    ~BlockAllocator() {
        Block *current = first;
        while (current) {
            Block *next = current->next;
            delete current;
            current = next;
        }
    }

    BlockAllocator &operator=(const BlockAllocator &) = delete;

    BlockAllocator &operator=(BlockAllocator &&other) noexcept {
        first = std::exchange(other.first, nullptr);
        last = std::exchange(other.last, nullptr);
        return *this;
    }

    unsigned char *alloc(unsigned size) {
        if (last->offset + size >= BLOCK_SIZE) {
            Block *new_block = new Block();
            last->next = new_block;
            last = new_block;
        }

        unsigned char *ptr = &last->data[last->offset];
        last->offset += Utils::align(size, 16);
        return ptr;
    }

    template <typename T, typename... Args>
    T *create(Args... args) {
        unsigned char *ptr = alloc(sizeof(T));
        return new (ptr) T(args...);
    }
};

#endif

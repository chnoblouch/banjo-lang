#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <vector>

template <typename T, int BlockSize>
class ArenaAllocator {

private:
    std::vector<T *> blocks;
    int index_in_block = BlockSize;

public:
    ~ArenaAllocator() {
        for (T *block : blocks) {
            for (int i = 0; i < BlockSize; i++) {
                blocks[i]->~T();
            }

            delete[] block;
        }
    }

    template <typename... Args>
    T *create(Args... args) {
        if (index_in_block == BlockSize) {
            allocate_block();
        }

        T *pointer = blocks.back()[index_in_block++];
        new (pointer) T(args...);
        return pointer;
    }

private:
    void allocate_block() {
        blocks.push_back(new T[BlockSize]);
        index_in_block = 0;
    }
};

#endif

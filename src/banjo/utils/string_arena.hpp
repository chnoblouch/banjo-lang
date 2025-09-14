#ifndef BANJO_UTILS_STRING_ARENA_H
#define BANJO_UTILS_STRING_ARENA_H

#include <algorithm>
#include <cstring>
#include <list>
#include <memory>
#include <string_view>

namespace banjo::utils {

template <unsigned MIN_BLOCK_SIZE>
class StringArena {

public:
    std::list<std::unique_ptr<char[]>> blocks;
    unsigned cur_block_size;
    unsigned cur_offset;

public:
    StringArena() : cur_offset(0) {}
    StringArena(const StringArena &) = delete;
    StringArena(StringArena &&) = default;

    StringArena &operator=(const StringArena &) = delete;
    StringArena &operator=(StringArena &&) = default;

    char *allocate_raw(unsigned length) {
        if (blocks.empty() || cur_offset + length > cur_block_size) {
            cur_block_size = std::max(MIN_BLOCK_SIZE, length);
            blocks.push_back(std::make_unique<char[]>(cur_block_size));
            cur_offset = 0;
        }

        std::unique_ptr<char[]> &cur_block = blocks.back();
        char *pointer = &cur_block[cur_offset];
        cur_offset += length;

        return pointer;
    }

    std::string_view allocate(unsigned length) {
        char *pointer = allocate_raw(length);
        return std::string_view(pointer, length);
    }

    std::string_view store(std::string_view string) {
        char *pointer = allocate_raw(string.size());
        std::memcpy(pointer, string.data(), string.size());
        return std::string_view(pointer, string.size());
    }
};

} // namespace banjo::utils

#endif

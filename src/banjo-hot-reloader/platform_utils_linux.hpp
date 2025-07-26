#ifndef BANJO_HOT_RELOADER_PLATFORM_UTILS_LINUX
#define BANJO_HOT_RELOADER_PLATFORM_UTILS_LINUX

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace banjo {

namespace hot_reloader {

class LinuxProcMaps {

public:
    struct Entry {
        std::uint64_t start_address;
        std::uint64_t end_address;
        std::string permissions;
        std::uint64_t offset;
        std::string device;
        std::uint64_t inode;
        std::string path;
    };

public:
    static LinuxProcMaps read(int pid);

private:
    std::ifstream stream;
    std::vector<Entry> entries;

public:
    const std::vector<Entry> &get_entries() const { return entries; }
    std::vector<Entry>::const_iterator begin() const { return entries.begin(); }
    std::vector<Entry>::const_iterator end() const { return entries.end(); }

private:
    LinuxProcMaps(int pid);
    Entry read_entry();

    std::string read_until(char delimiter);
    void skip_whitespace();

    static std::uint64_t parse_u64(const std::string &value, int base);
};

} // namespace hot_reloader

} // namespace banjo

#endif

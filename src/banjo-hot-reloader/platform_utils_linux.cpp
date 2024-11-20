#include "platform_utils_linux.hpp"

#include <cstdio>
#include <string>

#include <fcntl.h>
#include <unistd.h>

namespace banjo {

namespace hot_reloader {

LinuxProcMaps LinuxProcMaps::read(int pid) {
    return LinuxProcMaps(pid);
}

LinuxProcMaps::LinuxProcMaps(int pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/maps";
    stream = std::ifstream(path.c_str());

    while (stream.peek() != EOF) {
        entries.push_back(read_entry());
    }
}

LinuxProcMaps::Entry LinuxProcMaps::read_entry() {
    Entry entry;

    entry.start_address = parse_u64(read_until('-'), 16);
    stream.get();
    entry.end_address = parse_u64(read_until(' '), 16);
    skip_whitespace();
    entry.permissions = read_until(' ');
    skip_whitespace();
    entry.offset = parse_u64(read_until(' '), 16);
    skip_whitespace();
    entry.device = read_until(' ');
    skip_whitespace();
    entry.inode = parse_u64(read_until(' '), 10);
    skip_whitespace();
    entry.path = read_until('\n');
    stream.get();

    return entry;
}

std::string LinuxProcMaps::read_until(char delimiter) {
    std::string buffer;

    while (stream.peek() != delimiter) {
        buffer += stream.get();
    }

    return buffer;
}

void LinuxProcMaps::skip_whitespace() {
    while (stream.peek() == ' ') {
        stream.get();
    }
}

std::uint64_t LinuxProcMaps::parse_u64(const std::string &value, int base) {
    return std::stoull(value, nullptr, base);
}

} // namespace hot_reloader

} // namespace banjo

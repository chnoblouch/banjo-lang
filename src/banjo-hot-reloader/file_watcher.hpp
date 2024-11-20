#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include "banjo/utils/platform.hpp"

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

#if OS_LINUX
#    include <sys/poll.h>
#endif

namespace banjo {

namespace hot_reloader {

class FileWatcher {

private:
    std::filesystem::path path;

    int inotify_fd;
    struct pollfd inotify_pollfd;
    std::unordered_map<int, std::string> watch_descriptors;

public:
    static std::optional<FileWatcher> open(const std::filesystem::path &path);

private:
    FileWatcher();

public:
    std::vector<std::filesystem::path> poll(unsigned timeout_ms);
    void close();

private:
    bool watch_recursively(const std::filesystem::path &dir_path);
};

} // namespace hot_reloader

} // namespace banjo

#endif

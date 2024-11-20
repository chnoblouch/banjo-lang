#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <filesystem>
#include <optional>
#include <vector>

namespace banjo {

namespace hot_reloader {

class FileWatcher {

public:
    struct PlatformData;

    std::filesystem::path path;
    PlatformData *platform_data;

    static std::optional<FileWatcher> open(const std::filesystem::path &path);

private:
    FileWatcher(std::filesystem::path path, PlatformData *platform_data);

public:
    std::optional<std::vector<std::filesystem::path>> poll(unsigned timeout_ms);
    void close();
};

} // namespace hot_reloader

} // namespace banjo

#endif

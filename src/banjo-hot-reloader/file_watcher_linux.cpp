#include "file_watcher.hpp"

#include <filesystem>
#include <unordered_map>

#include <linux/limits.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <unistd.h>
#include <utility>

namespace banjo {

namespace hot_reloader {

struct FileWatcher::PlatformData {
    int inotify_fd;
    struct pollfd inotify_pollfd;
    std::unordered_map<int, std::string> watch_descriptors;
};

static bool watch_recursively(FileWatcher::PlatformData *platform_data, const std::filesystem::path &dir_path) {
    std::string path_string = dir_path.c_str();

    int watch_descriptor = inotify_add_watch(platform_data->inotify_fd, path_string.c_str(), IN_MODIFY);
    if (watch_descriptor == -1) {
        return false;
    }

    platform_data->watch_descriptors.insert({watch_descriptor, path_string});

    for (const std::filesystem::path &subdir_path : std::filesystem::directory_iterator(dir_path)) {
        if (!std::filesystem::is_directory(subdir_path)) {
            continue;
        }

        if (!watch_recursively(platform_data, subdir_path)) {
            return false;
        }
    }

    return true;
}

std::optional<FileWatcher> FileWatcher::open(const std::filesystem::path &path) {
    PlatformData *platform_data = new PlatformData();

    platform_data->inotify_fd = inotify_init();
    if (platform_data->inotify_fd == -1) {
        return {};
    }

    if (!watch_recursively(platform_data, path)) {
        return {};
    }

    platform_data->inotify_pollfd = {platform_data->inotify_fd, POLLIN, 0};

    return FileWatcher(path, platform_data);
}

FileWatcher::FileWatcher(std::filesystem::path path, PlatformData *platform_data)
  : path(std::move(path)),
    platform_data(platform_data) {}

std::optional<std::vector<std::filesystem::path>> FileWatcher::poll(unsigned timeout_ms) {
    std::vector<std::filesystem::path> paths;
    
    if (::poll(&platform_data->inotify_pollfd, 1, timeout_ms) <= 0) {
        return paths;
    }

    char buffer[4096];

    int length = read(platform_data->inotify_fd, buffer, sizeof(buffer));
    if (length == -1) {
        return {};
    }

    int i = 0;

    while (i < length) {
        struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);
        if (event->wd == -1 || event->mask & IN_Q_OVERFLOW) {
            continue;
        }

        if (event->mask & IN_MODIFY) {
            paths.push_back(std::filesystem::path(platform_data->watch_descriptors[event->wd]) / event->name);
        }

        i += sizeof(struct inotify_event) + length;
    }

    return paths;
}

void FileWatcher::close() {
    for (const auto &[watch_descriptor, path] : platform_data->watch_descriptors) {
        inotify_rm_watch(platform_data->inotify_fd, watch_descriptor);
    }

    ::close(platform_data->inotify_fd);
    delete platform_data;
}

} // namespace hot_reloader

} // namespace banjo

#include "file_watcher.hpp"

#include <filesystem>

#include <linux/limits.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <unistd.h>

namespace banjo {

namespace hot_reloader {

FileWatcher::FileWatcher() {}

std::optional<FileWatcher> FileWatcher::open(const std::filesystem::path &path) {
    FileWatcher watcher;

    watcher.inotify_fd = inotify_init();
    if (watcher.inotify_fd == -1) {
        return {};
    }

    if (!watcher.watch_recursively(path)) {
        return {};
    }

    watcher.inotify_pollfd = {watcher.inotify_fd, POLLIN, 0};
    return watcher;
}

std::vector<std::filesystem::path> FileWatcher::poll(unsigned timeout_ms) {
    if (::poll(&inotify_pollfd, 1, timeout_ms) <= 0) {
        return {};
    }

    char buffer[4096];

    int length = read(inotify_fd, buffer, sizeof(buffer));
    if (length == -1) {
        return {};
    }

    std::vector<std::filesystem::path> paths;
    int i = 0;

    while (i < length) {
        struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);
        if (event->wd == -1 || event->mask & IN_Q_OVERFLOW) {
            continue;
        }

        if (event->mask & IN_MODIFY) {
            paths.push_back(std::filesystem::path(watch_descriptors[event->wd]) / event->name);
        }

        i += sizeof(struct inotify_event) + length;
    }

    return paths;
}

bool FileWatcher::watch_recursively(const std::filesystem::path &dir_path) {
    std::string path_string = dir_path.c_str();

    int watch_descriptor = inotify_add_watch(inotify_fd, path_string.c_str(), IN_MODIFY);
    if (watch_descriptor == -1) {
        return false;
    }

    watch_descriptors.insert({watch_descriptor, path_string});

    for (const std::filesystem::path &subdir_path : std::filesystem::directory_iterator(dir_path)) {
        if (!std::filesystem::is_directory(subdir_path)) {
            continue;
        }

        if (!watch_recursively(subdir_path)) {
            return false;
        }
    }

    return true;
}

void FileWatcher::close() {
    for (const auto &[watch_descriptor, path] : watch_descriptors) {
        inotify_rm_watch(inotify_fd, watch_descriptor);
    }

    ::close(inotify_fd);
}

} // namespace hot_reloader

} // namespace banjo

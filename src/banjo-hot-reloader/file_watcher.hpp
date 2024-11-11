#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>

namespace banjo {

namespace hot_reloader {

class FileWatcher {

private:
    typedef std::chrono::file_clock Clock;
    typedef std::chrono::time_point<Clock> TimePoint;
    typedef std::chrono::duration<Clock> Duration;

    std::filesystem::path path;
    std::function<void(const std::filesystem::path &file_path)> on_changed;
    std::unordered_map<std::string, TimePoint> last_change_times;
    std::thread thread;
    std::atomic<bool> running = false;

public:
    FileWatcher(std::filesystem::path path, std::function<void(std::filesystem::path file_path)> on_changed);
    void stop();

private:
    void run();
    void handle_file_change(const std::string &name, std::vector<std::filesystem::path> &changed_files);
};

} // namespace hot_reloader

} // namespace banjo

#endif

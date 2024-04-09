#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>

#include <windows.h>

class FileWatcher {

private:
    typedef std::chrono::file_clock Clock;
    typedef std::chrono::time_point<Clock> TimePoint;
    typedef std::chrono::duration<Clock> Duration;

    HANDLE dir_handle;
    OVERLAPPED overlapped;
    char change_buffer[2048];

    std::filesystem::path path;
    std::function<void(std::filesystem::path file_path)> on_changed;
    std::unordered_map<std::string, TimePoint> last_change_times;
    std::thread thread;
    bool running = false;

public:
    FileWatcher(std::filesystem::path path, std::function<void(std::filesystem::path file_path)> on_changed);
    void stop();

private:
    void run();
    void handle_file_change(std::string name, std::vector<std::filesystem::path> &changed_files);
};

#endif

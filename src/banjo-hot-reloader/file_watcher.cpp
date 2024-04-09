// clang-format off
#include <windows.h>
#include "file_watcher.hpp"
// clang-format on

#include "diagnostics.hpp"

constexpr unsigned MIN_TIME_BETEWEEN_CHANGES_MS = 500;

FileWatcher::FileWatcher(std::filesystem::path path, std::function<void(std::filesystem::path file_path)> on_changed)
  : path(path),
    on_changed(on_changed),
    thread([this]() { run(); }) {}

void FileWatcher::run() {
    std::string canonical_path = std::filesystem::canonical(path).string();
    Diagnostics::log("watching directory '" + canonical_path + "'");

    dir_handle = CreateFile(
        canonical_path.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    ZeroMemory(&overlapped, sizeof(OVERLAPPED));
    overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

    BOOL result = ReadDirectoryChangesW(
        dir_handle,
        change_buffer,
        sizeof(change_buffer) / sizeof(char),
        TRUE,
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL,
        &overlapped,
        NULL
    );

    if (!result) {
        Diagnostics::abort("failed to create file watcher");
    }

    running = true;

    while (running) {
        DWORD status = WaitForSingleObject(overlapped.hEvent, 25);
        if (status != WAIT_OBJECT_0) {
            continue;
        }

        DWORD bytes_transferred;
        GetOverlappedResult(dir_handle, &overlapped, &bytes_transferred, FALSE);
        FILE_NOTIFY_INFORMATION *event = (FILE_NOTIFY_INFORMATION *)change_buffer;

        std::vector<std::filesystem::path> changed_files;

        while (true) {
            std::string file_name(event->FileName, event->FileName + event->FileNameLength / sizeof(wchar_t));
            handle_file_change(file_name, changed_files);

            if (event->NextEntryOffset) {
                event = (FILE_NOTIFY_INFORMATION *)((char *)event + event->NextEntryOffset);
            } else {
                break;
            }
        }

        for (const std::filesystem::path &changed_file : changed_files) {
            on_changed(changed_file);
        }

        result = ReadDirectoryChangesW(
            dir_handle,
            change_buffer,
            sizeof(change_buffer) / sizeof(char),
            FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            NULL,
            &overlapped,
            NULL
        );

        if (!result) {
            Diagnostics::abort("failed to recreate file watcher");
        }
    }
}

void FileWatcher::handle_file_change(std::string file_name, std::vector<std::filesystem::path> &changed_files) {
    TimePoint now = Clock::now();

    if (last_change_times.contains(file_name)) {
        auto delta = now - last_change_times[file_name];
        auto delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delta);

        if (delta_ms.count() < MIN_TIME_BETEWEEN_CHANGES_MS) {
            return;
        }
    }

    last_change_times[file_name] = now;
    Diagnostics::log("file '" + file_name + "' has changed");
    changed_files.push_back(path / file_name);
}

void FileWatcher::stop() {
    running = false;
    thread.join();
}

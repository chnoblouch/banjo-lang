#include "file_watcher.hpp"

#include <filesystem>
#include <utility>

#include <windows.h>

namespace banjo {

namespace hot_reloader {

struct FileWatcher::PlatformData {
    HANDLE dir_handle;
    OVERLAPPED overlapped;
    char change_buffer[2048];
};

std::optional<FileWatcher> FileWatcher::open(const std::filesystem::path &path) {
    std::string path_string = path.string();
    PlatformData *platform_data = new PlatformData();

    platform_data->dir_handle = CreateFile(
        path_string.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (platform_data->dir_handle == INVALID_HANDLE_VALUE) {
        return {};
    }

    ZeroMemory(&platform_data->overlapped, sizeof(OVERLAPPED));
    platform_data->overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

    BOOL result = ReadDirectoryChangesW(
        platform_data->dir_handle,
        platform_data->change_buffer,
        sizeof(platform_data->change_buffer),
        TRUE,
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL,
        &platform_data->overlapped,
        NULL
    );

    if (!result) {
        return {};
    }

    return FileWatcher(path, platform_data);
}

FileWatcher::FileWatcher(std::filesystem::path path, PlatformData *platform_data)
  : path(std::move(path)),
    platform_data(platform_data) {}

std::optional<std::vector<std::filesystem::path>> FileWatcher::poll(unsigned timeout_ms) {
    std::vector<std::filesystem::path> paths;

    DWORD status = WaitForSingleObject(platform_data->overlapped.hEvent, timeout_ms);
    if (status == WAIT_FAILED) {
        return {};
    }

    if (status != WAIT_OBJECT_0) {
        return paths;
    }

    DWORD bytes_transferred;
    BOOL result = GetOverlappedResult(platform_data->dir_handle, &platform_data->overlapped, &bytes_transferred, FALSE);
    if (!result) {
        return {};
    }

    FILE_NOTIFY_INFORMATION *event = (FILE_NOTIFY_INFORMATION *)platform_data->change_buffer;

    while (true) {
        std::string file_name(event->FileName, event->FileName + event->FileNameLength / sizeof(wchar_t));

        std::filesystem::path file_path = path / std::filesystem::path(file_name);
        file_path.make_preferred();
        paths.push_back(file_path);

        if (event->NextEntryOffset) {
            event = (FILE_NOTIFY_INFORMATION *)((char *)event + event->NextEntryOffset);
        } else {
            break;
        }
    }

    result = ReadDirectoryChangesW(
        platform_data->dir_handle,
        platform_data->change_buffer,
        sizeof(platform_data->change_buffer),
        FALSE,
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        NULL,
        &platform_data->overlapped,
        NULL
    );

    if (!result) {
        return {};
    }

    return paths;
}

void FileWatcher::close() {
    CloseHandle(platform_data->dir_handle);
    delete platform_data;
}

} // namespace hot_reloader

} // namespace banjo

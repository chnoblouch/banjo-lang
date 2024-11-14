#include "target_process.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>

// clang-format off
#include <windows.h>
#include <dbghelp.h>
#include <Psapi.h>
// clang-format on

namespace banjo {

namespace hot_reloader {

std::optional<TargetProcess> TargetProcess::spawn(std::string executable) {
    STARTUPINFO startup_info;
    ZeroMemory(&startup_info, sizeof(STARTUPINFO));

    PROCESS_INFORMATION process_info;
    BOOL result = CreateProcess(
        NULL,
        executable.data(),
        NULL,
        NULL,
        FALSE,
        DEBUG_PROCESS,
        NULL,
        NULL,
        &startup_info,
        &process_info
    );

    if (result) {
        return TargetProcess(std::move(executable), process_info.hProcess, process_info.hThread);
    } else {
        return {};
    }
}

TargetProcess::TargetProcess(std::string executable, HANDLE process, HANDLE thread)
  : executable(std::move(executable)),
    process(process),
    thread(thread),
    state(State::INITIALIZING) {}

void TargetProcess::poll() {
    DEBUG_EVENT win_event;
    if (!WaitForDebugEvent(&win_event, INFINITE)) {
        return;
    }

    if (win_event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
        if (state == State::INITIALIZING) {
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
            SymInitialize(process, NULL, TRUE);
        }

        state = State::RUNNING;
    } else if (win_event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
        state = State::EXITED;
    }

    switch (win_event.dwDebugEventCode) {
        case CREATE_THREAD_DEBUG_EVENT: state = State::RUNNING; break;
        case EXIT_PROCESS_DEBUG_EVENT: state = State::EXITED; break;
        default: break;
    }

    ContinueDebugEvent(win_event.dwProcessId, win_event.dwThreadId, DBG_CONTINUE);
}

std::optional<TargetProcess::Address> TargetProcess::find_section(std::string_view name) {
    BOOL result;

    CHAR win_image_path[MAX_PATH + 1];
    DWORD win_image_path_size = sizeof(win_image_path) / sizeof(CHAR);
    result = QueryFullProcessImageName(process, 0, win_image_path, &win_image_path_size);

    if (!result) {
        return {};
    }

    std::string image_path(win_image_path);
    std::ifstream stream(image_path);

    // Read the offset of the signature in the MS-DOS stub and skip it to get to the COFF header.
    stream.seekg(0x3C, std::ios::beg);
    std::uint32_t signature_offset;
    stream.read(reinterpret_cast<char *>(&signature_offset), 4);
    stream.seekg(signature_offset + 4);

    // Read the number of sections.
    stream.seekg(2, std::ios::cur);
    std::uint16_t section_count;
    stream.read(reinterpret_cast<char *>(&section_count), 2);

    // Read the size of the optional header.
    stream.seekg(12, std::ios::cur);
    std::uint16_t optional_header_size;
    stream.read(reinterpret_cast<char *>(&optional_header_size), 2);

    // Skip the rest of the COFF header as well as the optional header to get to the section table.
    stream.seekg(2 + optional_header_size, std::ios::cur);

    std::optional<std::uint32_t> virtual_address;

    // Find the requested section in the section table.
    for (std::uint16_t i = 0; i < section_count; i++) {
        // Read the section name.
        char name_data[9];
        stream.read(name_data, 8);
        name_data[8] = '\0';

        if (std::string_view(name_data) == name) {
            // Read the virtual address.
            stream.seekg(4, std::ios::cur);
            virtual_address = 0;
            stream.read(reinterpret_cast<char *>(&virtual_address.value()), 4);
            break;
        }

        // Move on to the next section header.
        stream.seekg(32, std::ios::cur);
    }

    HMODULE modules[1024];
    DWORD modules_size;
    result = EnumProcessModules(process, modules, sizeof(modules), &modules_size);

    if (!result) {
        return {};
    }

    HMODULE image_module = NULL;

    for (unsigned i = 0; i < modules_size / sizeof(HMODULE); i++) {
        TCHAR win_module_path[MAX_PATH + 1];
        result = GetModuleFileNameEx(process, modules[i], win_module_path, sizeof(win_module_path) / sizeof(TCHAR));

        if (!result) {
            continue;
        }

        std::string module_path(win_module_path);
        if (module_path == image_path) {
            image_module = modules[i];
            break;
        }
    }

    if (!image_module) {
        return {};
    }

    MODULEINFO module_info;
    result = GetModuleInformation(process, image_module, &module_info, sizeof(module_info));

    if (!result) {
        return {};
    }

    TargetProcess::Address base_address = reinterpret_cast<TargetProcess::Address>(module_info.lpBaseOfDll);
    return base_address + (*virtual_address);
}

std::optional<TargetProcess::Address> TargetProcess::allocate_memory(Size size, MemoryProtection protection) {
    DWORD win_protect;

    switch (protection) {
        case MemoryProtection::READ_WRITE: win_protect = PAGE_READWRITE; break;
        case MemoryProtection::READ_WRITE_EXECUTE: win_protect = PAGE_EXECUTE_READWRITE; break;
    }

    LPVOID addr = VirtualAllocEx(process, NULL, static_cast<SIZE_T>(size), MEM_COMMIT | MEM_RESERVE, win_protect);

    if (addr) {
        return reinterpret_cast<Address>(addr);
    } else {
        return {};
    }
}

bool TargetProcess::write_memory(Address address, const void *data, std::size_t size) {
    BOOL result = WriteProcessMemory(
        process,
        reinterpret_cast<LPVOID>(address),
        static_cast<LPCVOID>(data),
        static_cast<SIZE_T>(size),
        NULL
    );

    return result;
}

void TargetProcess::close() {
    WaitForSingleObject(process, INFINITE);
    CloseHandle(process);
    CloseHandle(thread);
}

} // namespace hot_reloader

} // namespace banjo

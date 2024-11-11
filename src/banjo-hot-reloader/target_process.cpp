#include "target_process.hpp"

// clang-format off
#include <windows.h>
#include <dbghelp.h>
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
        return TargetProcess(process_info.hProcess, process_info.hThread);
    } else {
        return {};
    }
}

TargetProcess::TargetProcess(HANDLE process, HANDLE thread)
  : process(process),
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

std::optional<TargetProcess::Address> TargetProcess::get_symbol_addr(const std::string &name) {
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];

    SYMBOL_INFO *symbol = (SYMBOL_INFO *)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    BOOL result = SymFromName(process, name.c_str(), symbol);

    if (result) {
        return static_cast<TargetProcess::Address>(symbol->Address);
    } else {
        return {};
    }
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

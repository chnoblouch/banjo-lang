#ifndef TARGET_PROCESS_H
#define TARGET_PROCESS_H

#include "banjo/utils/platform.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#if OS_WINDOWS
typedef unsigned long DWORD;
typedef void *HANDLE;
#endif

namespace banjo {

namespace hot_reloader {

class TargetProcess {

public:
    typedef std::uint64_t Address;
    typedef std::uint64_t Size;

    enum class State {
        INITIALIZING,
        RUNNING,
        EXITED,
    };

    enum class MemoryProtection {
        READ_WRITE,
        READ_WRITE_EXECUTE,
    };

private:
    std::string executable;

#if OS_WINDOWS
    HANDLE process;
    HANDLE thread;
#elif OS_LINUX
    int process;
#endif

    State state;

public:
    static std::optional<TargetProcess> spawn(std::string executable);

private:
#ifdef OS_WINDOWS
    TargetProcess(std::string executable, HANDLE process, HANDLE thread);
#elif OS_LINUX
    TargetProcess(std::string executable, int process);
#endif

public:
    State get_state() { return state; }
    bool is_running() { return state == State::RUNNING; }
    
    void poll();
    std::optional<TargetProcess::Address> find_section(std::string_view name);
    std::optional<Address> allocate_memory(Size size, MemoryProtection protection);
    bool read_memory(Address address, void *buffer, Size size);
    bool write_memory(Address address, const void *buffer, Size size);
    bool is_exited();
    void close();
};

} // namespace hot_reloader

} // namespace banjo

#endif

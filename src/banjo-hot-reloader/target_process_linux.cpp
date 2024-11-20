#include "banjo/utils/utils.hpp"
#include "target_process.hpp"

#include "banjo/utils/macros.hpp"
#include "banjo/utils/write_buffer.hpp"
#include "file_reader.hpp"
#include "platform_utils_linux.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <sys/syscall.h>
#include <vector>

#include <linux/limits.h>
#include <linux/prctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

namespace banjo {

namespace hot_reloader {

std::optional<TargetProcess> TargetProcess::spawn(std::string executable) {
    int parent_pid = getpid();

    int fork_result = vfork();
    if (fork_result == -1) {
        return {};
    }

    if (fork_result == 0) {
        // Kill the child as well if the parent dies.
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1) {
            exit(1);
        }

        // The parent might have died already, so check for that.
        if (getppid() != parent_pid) {
            exit(1);
        }

        char *argv[] = {executable.data(), NULL};
        if (execve(executable.data(), argv, environ) == -1) {
            exit(1);
        }

        ASSERT_UNREACHABLE;
    }

    int process = fork_result;
    return TargetProcess(std::move(executable), process);
}

TargetProcess::TargetProcess(std::string executable, int process)
  : executable(std::move(executable)),
    exited(false),
    process(process) {}

void TargetProcess::poll() {
    // FIXME: I think this might break if another process with the same pid is spawned
    // between calls to `is_exited`, though this should be extremely unlikely.

    int status;
    if (waitpid(process, &status, WNOHANG) <= 0) {
        return;
    }

    if (WIFEXITED(status)) {
        exited = true;
    }
}

std::optional<TargetProcess::Address> TargetProcess::find_section(std::string_view name) {
    // Get the page size of the system for aligning addresses.
    int page_size = getpagesize();

    // Get the path to the executable using the virtual `/proc/$pid/exe` file.
    std::string proc_exe_path = "/proc/" + std::to_string(process) + "/exe";
    char executable_path[PATH_MAX + 1];
    size_t length = readlink(proc_exe_path.c_str(), executable_path, PATH_MAX);
    executable_path[length] = '\0';

    // Open the executable ELF file.
    FileReader reader(executable_path);

    // Read the ELF file header.
    reader.seek(32);
    std::uint64_t segment_table_offset = reader.read_u64();
    std::uint64_t section_table_offset = reader.read_u64();
    reader.skip(6);
    std::uint16_t segment_header_size = reader.read_u16();
    std::uint16_t num_segments = reader.read_u16();
    std::uint16_t section_header_size = reader.read_u16();
    std::uint16_t num_sections = reader.read_u16();
    std::uint16_t shstrtab_index = reader.read_u16();

    // Read the data offset of the section name table section.
    reader.seek(section_table_offset);
    reader.skip(shstrtab_index * section_header_size + 24);
    std::uint64_t shstrtab_data_pos = reader.read_u64();

    bool found_section_data_offset = false;
    std::uint64_t section_data_offset;

    // Iterate over all sections until one is found with the requested name.
    for (unsigned i = 0; i < num_sections; i++) {
        // Move to the section header with the current index in the section header table.
        reader.seek(section_table_offset + i * section_header_size);

        std::uint32_t name_offset = reader.read_u32();
        std::uint64_t pos = reader.tell();

        // Move to the offset of the section name in the section name table (.shstrtab).
        reader.seek(shstrtab_data_pos + name_offset);

        std::string section_name;
        std::uint8_t c;

        // Read the section name from the section name table.
        while ((c = reader.read_u8()) != 0) {
            section_name.push_back(c);
        }

        // If this is the section we're looking for, read the offset of its data in the file.
        if (section_name == name) {
            reader.seek(pos);
            reader.skip(20);
            section_data_offset = reader.read_u64();
            found_section_data_offset = true;
            break;
        }
    }

    if (!found_section_data_offset) {
        return {};
    }

    bool found_segment = false;
    std::uint64_t segment_virtual_addr;

    // Iterate over all segments until the segment is found that contains the section.
    for (unsigned i = 0; i < num_segments; i++) {
        // Move to the program header with the current index in the program header table.
        reader.seek(segment_table_offset + i * segment_header_size);

        reader.skip(8);
        std::uint64_t data_offset = reader.read_u64();
        reader.skip(8);
        std::uint64_t virtual_addr = reader.read_u64();
        std::uint32_t data_size = reader.read_u32();

        // Check if the section data offset is inside this segment.
        if (section_data_offset >= data_offset && section_data_offset < data_offset + data_size) {
            segment_virtual_addr = virtual_addr;
            found_segment = true;
            break;
        }
    }

    if (!found_segment) {
        return {};
    }

    // Calculate the mapping address for this segment by truncating its virtual address to the page size of the system.
    // I'm not really sure if this is correct according to the ELF standard, but it has worked out so far.
    // For reference: https://docs.oracle.com/cd/E19683-01/817-3677/chapter6-37/index.html
    std::uint64_t mapping_address = segment_virtual_addr & ~(page_size - 1);

    bool found_mapping_offset = false;
    std::uint64_t mapping_offset;

    // Find out at which offset in the file the segment was mapped using the virtual `/proc/$pid/maps` file.
    for (const LinuxProcMaps::Entry &entry : LinuxProcMaps::read(process)) {
        if (entry.path == executable_path && entry.start_address == mapping_address) {
            mapping_offset = entry.offset;
            found_mapping_offset = true;
            break;
        }
    }

    if (!found_mapping_offset) {
        return {};
    }

    // Calculate the final section address.
    // First add the offset of the section in the file to the mapping address for the segment.
    // The loader might not map from the start of the file however, so subtract the offset that was used.
    return mapping_address + section_data_offset - mapping_offset;
}

std::optional<TargetProcess::Address> TargetProcess::allocate_memory(Size size, MemoryProtection protection) {
    // Get the page size of the system for aligning allocations.
    int page_size = getpagesize();

    // Attach to the process and stop it.
    if (ptrace(PTRACE_ATTACH, process, 0, 0) == -1) {
        return {};
    }
    waitpid(process, NULL, 0);

    // Back up the current register values.
    struct user_regs_struct orig_regs;
    if (ptrace(PTRACE_GETREGS, process, 0, &orig_regs) == -1) {
        return {};
    }

    // Back up the current 8 bytes at the instruction pointer.
    long orig_word = ptrace(PTRACE_PEEKDATA, process, orig_regs.rip, 0);

    // Convert the page protection.
    int permissions;
    switch (protection) {
        case MemoryProtection::READ_WRITE: permissions = PROT_READ | PROT_WRITE; break;
        case MemoryProtection::READ_WRITE_EXECUTE: permissions = PROT_READ | PROT_WRITE | PROT_EXEC; break;
    }

    // Prepare the arguments for the mmap system call.
    struct user_regs_struct new_regs = orig_regs;
    new_regs.rax = SYS_mmap;                      // Set system call number to sys_mmap.
    new_regs.rdi = 0x0;                           // Set `addr` parameter to NULL.
    new_regs.rsi = Utils::align(size, page_size); // Set the allocation size (a multiple of the page size).
    new_regs.rdx = permissions;                   // Set the memory protection of the pages.
    new_regs.r10 = MAP_PRIVATE | MAP_ANONYMOUS;   // Create a private mapping not backed by a file (anonymous).
    new_regs.r8 = -1;                             // Set fd to -1 because this is an anonymous mapping.
    new_regs.r9 = 0;                              // Set the file offset to zero.

    // Replace the register values in the target process with the system call arguments.
    if (ptrace(PTRACE_SETREGS, process, 0, &new_regs) == -1) {
        return {};
    }

    // Prepare the syscall instruction (`syscall` or 0x0F, 0x05) followed by six trap instructions (`int3` or 0xCC).
    // One trap instruction would have been enough, but with PTRACE_POKEDATA we can only write a multiple of 8 bytes
    // because that's the word size of x86-64.
    unsigned char instructions[8] = {0x0F, 0x05, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
    long word;
    std::memcpy(&word, instructions, 8);

    // Replace the current 8 bytes at the instruction pointer with the syscall and trap instructions.
    if (ptrace(PTRACE_POKEDATA, process, orig_regs.rip, word) == -1) {
        return {};
    }

    // Continue the process until it hits the first trap instruction (this causes an interrupt).
    if (ptrace(PTRACE_CONT, process, 0, 0) == -1) {
        return {};
    }
    waitpid(process, NULL, 0);

    // The mmap system call has now been completed and the registers have been updated by the kernel.
    struct user_regs_struct result_regs = orig_regs;
    if (ptrace(PTRACE_GETREGS, process, 0, &result_regs) == -1) {
        return {};
    }

    // Copy the address returned by the system call from the rax register.
    std::int64_t address;
    std::memcpy(&address, &result_regs.rax, 8);

    // Restore the original register values.
    if (ptrace(PTRACE_SETREGS, process, 0, &orig_regs) == -1) {
        return {};
    }

    // Restore the original 8 bytes at the instruction pointer.
    if (ptrace(PTRACE_POKEDATA, process, orig_regs.rip, orig_word) == -1) {
        return {};
    }

    // Detach from the process to continue it.
    if (ptrace(PTRACE_DETACH, process, 0, 0) == -1) {
        return {};
    }

    return address;
}

bool TargetProcess::read_memory(Address address, void *buffer, Size size) {
    if (ptrace(PTRACE_ATTACH, process, 0, 0) == -1) {
        return false;
    }

    waitpid(process, NULL, 0);

    std::ifstream stream("/proc/" + std::to_string(process) + "/mem");
    stream.seekg(address, std::ios::beg);
    stream.read(reinterpret_cast<char *>(buffer), size);
    bool result = stream.good();
    stream.close();

    if (ptrace(PTRACE_DETACH, process, 0, 0) == -1) {
        return false;
    }

    return result;
}

bool TargetProcess::write_memory(Address address, const void *buffer, Size size) {
    if (ptrace(PTRACE_ATTACH, process, 0, 0) == -1) {
        return false;
    }

    waitpid(process, NULL, 0);

    std::ofstream stream("/proc/" + std::to_string(process) + "/mem");
    stream.seekp(address, std::ios::beg);
    stream.write(reinterpret_cast<const char *>(buffer), size);
    bool result = stream.good();
    stream.close();

    if (ptrace(PTRACE_DETACH, process, 0, 0) == -1) {
        return false;
    }

    return result;
}

void TargetProcess::close() {}

} // namespace hot_reloader

} // namespace banjo

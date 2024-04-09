#ifndef TARGET_PROCESS_H
#define TARGET_PROCESS_H

#include "emit/binary_module.hpp"
#include <string>

typedef unsigned long DWORD;
typedef void *HANDLE;

class TargetProcess {

public:
    struct LoadedFunc {
        void *text_addr;
        std::size_t text_size;
        void *data_addr;
        std::size_t data_size;
    };

private:
    HANDLE process;
    HANDLE thread;

public:
    TargetProcess(HANDLE process, HANDLE thread);

    HANDLE get_process() { return process; }
    HANDLE get_thread() { return thread; }

    void init_symbols();
    void *get_symbol_addr(const std::string &name);
    void *read_func_ptr(unsigned index);
    LoadedFunc load_func(BinModule &module_);
    void close();

private:
    void *alloc_section(std::size_t size, DWORD protection);
    void resolve_symbol_use(BinModule &module_, const LoadedFunc &loaded_func, const BinSymbolUse &use);
    void write_section(void *address, const WriteBuffer &buffer);
};

#endif

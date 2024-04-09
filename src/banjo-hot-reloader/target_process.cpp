#include "target_process.hpp"

#include "diagnostics.hpp"

#include <windows.h>

#include <cstdint>
#include <dbghelp.h>

TargetProcess::TargetProcess(HANDLE process, HANDLE thread) : process(process), thread(thread) {}

void TargetProcess::init_symbols() {
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize(process, NULL, TRUE);
}

void *TargetProcess::get_symbol_addr(const std::string &name) {
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];

    SYMBOL_INFO *symbol = (SYMBOL_INFO *)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    if (!SymFromName(process, name.c_str(), symbol)) {
        Diagnostics::log("warning: couldn't find symbol '" + name + "'");
        return nullptr;
    }

    return (void *)symbol->Address;
}

void *TargetProcess::read_func_ptr(unsigned index) {
    void *addr_table_ptr = get_symbol_addr("addr_table");
    void *item_addr = (void *)((std::intptr_t)addr_table_ptr + 8 * index);

    void *address;
    SIZE_T num_bytes_read;
    BOOL result = ReadProcessMemory(process, item_addr, &address, 8, &num_bytes_read);

    if (!result || num_bytes_read != 8) {
        Diagnostics::abort("failed to read item from address table");
    }

    return address;
}

TargetProcess::LoadedFunc TargetProcess::load_func(BinModule &module_) {
    std::size_t text_size = module_.text.get_size();
    std::size_t data_size = module_.data.get_size();

    LoadedFunc loaded_func{
        .text_addr = alloc_section(text_size, PAGE_EXECUTE_READWRITE),
        .text_size = text_size,
        .data_addr = alloc_section(data_size, PAGE_READWRITE),
        .data_size = data_size,
    };

    for (const BinSymbolUse &use : module_.symbol_uses) {
        resolve_symbol_use(module_, loaded_func, use);
    }

    write_section(loaded_func.text_addr, module_.text);
    write_section(loaded_func.data_addr, module_.data);

    return loaded_func;
}

void TargetProcess::resolve_symbol_use(BinModule &module_, const LoadedFunc &loaded_func, const BinSymbolUse &use) {
    std::intptr_t use_addr = 0;
    if (use.section == BinSectionKind::TEXT) {
        use_addr = (std::intptr_t)loaded_func.text_addr + use.address;
    } else if (use.section == BinSectionKind::DATA) {
        use_addr = (std::intptr_t)loaded_func.data_addr + use.address;
    }

    const BinSymbolDef &def = module_.symbol_defs[use.symbol_index];
    std::intptr_t def_addr = 0;

    if (def.name == "addr_table") {
        def_addr = (std::intptr_t)get_symbol_addr("addr_table");
    } else if (def.kind == BinSymbolKind::DATA_LABEL) {
        def_addr = (std::intptr_t)loaded_func.data_addr + def.offset;
    }

    if (use.kind == BinSymbolUseKind::REL32) {
        std::ptrdiff_t offset = def_addr - (use_addr + 4);
        module_.text.seek(use.address);
        module_.text.write_i32(offset);
    } else if (use.kind == BinSymbolUseKind::ABS64) {
        if (use.section == BinSectionKind::TEXT) {
            module_.text.seek(use.address);
            module_.text.write_u64(def_addr);
        } else if (use.section == BinSectionKind::DATA) {
            module_.data.seek(use.address);
            module_.data.write_u64(def_addr);
        }
    }
}

void *TargetProcess::alloc_section(std::size_t size, DWORD protection) {
    if (size == 0) {
        return nullptr;
    }

    void *addr = VirtualAllocEx(process, NULL, size, MEM_COMMIT | MEM_RESERVE, protection);
    if (!addr) {
        Diagnostics::abort("failed to allocate memory for section");
    }

    return addr;
}

void TargetProcess::write_section(void *address, const WriteBuffer &buffer) {
    if (buffer.get_size() == 0) {
        return;
    }

    if (!WriteProcessMemory(process, address, buffer.get_data().data(), buffer.get_size(), NULL)) {
        Diagnostics::abort("failed to write section");
    }
}

void TargetProcess::close() {
    WaitForSingleObject(process, INFINITE);
    CloseHandle(process);
    CloseHandle(thread);
}

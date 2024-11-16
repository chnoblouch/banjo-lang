#include "hot_reloader.hpp"

#include "banjo/ssa_gen/name_mangling.hpp"
#include "diagnostics.hpp"
#include "file_watcher.hpp"
#include "jit_compiler.hpp"
#include "target_process.hpp"

namespace banjo {

namespace hot_reloader {

HotReloader::HotReloader() {
    Diagnostics::log("platform: x86_64-windows");
}

void HotReloader::run(const std::string &executable, const std::filesystem::path &src_path) {
    process = TargetProcess::spawn(executable);

    if (process) {
        Diagnostics::log("executable loaded");
    } else {
        Diagnostics::abort("failed to load executable");
    }

    while (!process->is_running()) {
        process->poll();
    }

    std::optional<TargetProcess::Address> pointer = process->find_section(".bnjatbl");

    if (pointer) {
        addr_table_ptr = *pointer;
        Diagnostics::log("found address table in target process");
    } else {
        Diagnostics::abort("failed to find address table in target process");
    }

    TargetProcess::Address target_addr = addr_table_ptr;

    std::uint32_t num_symbols;
    if (process->read_memory(target_addr, &num_symbols, 4)) {
        target_addr += 4;
    } else {
        Diagnostics::abort("failed to read number of symbols");
    }

    for (unsigned i = 0; i < num_symbols; i++) {
        std::uint32_t symbol_length;
        if (process->read_memory(target_addr, &symbol_length, 4)) {
            target_addr += 4;
        } else {
            Diagnostics::abort("failed to read symbol length");
        }

        std::string symbol_name(symbol_length, '\0');
        if (process->read_memory(target_addr, symbol_name.data(), symbol_length)) {
            target_addr += symbol_length;
        } else {
            Diagnostics::abort("failed to read symbol name");
        }

        addr_table.append(symbol_name);
    }

    Diagnostics::log("address table layout loaded (" + std::to_string(num_symbols) + " symbols)");

    FileWatcher watcher(src_path, std::bind(&HotReloader::reload_file, this, std::placeholders::_1));

    while (!process->is_exited()) {
        process->poll();
    }

    watcher.stop();
    process->close();
    Diagnostics::log("process exited");
}

void HotReloader::reload_file(const std::filesystem::path &file_path) {
    JITCompiler compiler(lang::Config::instance(), addr_table);
    if (!compiler.build_ir()) {
        Diagnostics::log("failed to reload file");
        return;
    }

    std::filesystem::path absolute_path = std::filesystem::absolute(file_path);
    lang::sir::Module *mod = compiler.find_mod(absolute_path);

    if (!mod) {
        return;
    }

    std::vector<lang::sir::FuncDef *> funcs;
    collect_funcs(mod->block, funcs);

    for (lang::sir::FuncDef *func : funcs) {
        std::string name = lang::NameMangling::mangle_func_name(*func);

        std::optional<unsigned> index = addr_table.find_index(name);
        if (!index) {
            continue;
        }

        BinModule mod = compiler.compile_func(name);
        LoadedFunc loaded_func = load_func(mod);
        update_func_addr(*func, *index, loaded_func.text_addr);
    }
}

void HotReloader::collect_funcs(lang::sir::DeclBlock &block, std::vector<lang::sir::FuncDef *> &out_funcs) {
    for (lang::sir::Decl decl : block.decls) {
        if (auto func_def = decl.match<lang::sir::FuncDef>()) {
            out_funcs.push_back(func_def);
        } else if (auto struct_def = decl.match<lang::sir::StructDef>()) {
            collect_funcs(struct_def->block, out_funcs);
        } else if (auto union_def = decl.match<lang::sir::UnionDef>()) {
            collect_funcs(union_def->block, out_funcs);
        }
    }
}

HotReloader::LoadedFunc HotReloader::load_func(BinModule &mod) {
    std::size_t text_size = mod.text.get_size();
    std::size_t data_size = mod.data.get_size();

    LoadedFunc loaded_func{
        .text_addr = alloc_section(text_size, TargetProcess::MemoryProtection::READ_WRITE_EXECUTE),
        .text_size = text_size,
        .data_addr = alloc_section(data_size, TargetProcess::MemoryProtection::READ_WRITE),
        .data_size = data_size,
    };

    for (const BinSymbolUse &use : mod.symbol_uses) {
        resolve_symbol_use(mod, loaded_func, use);
    }

    write_section(loaded_func.text_addr, mod.text);
    write_section(loaded_func.data_addr, mod.data);

    return loaded_func;
}

TargetProcess::Address HotReloader::alloc_section(
    TargetProcess::Size size,
    TargetProcess::MemoryProtection protection
) {
    if (size == 0) {
        return 0;
    }

    std::optional<TargetProcess::Address> addr = process->allocate_memory(size, protection);

    if (addr) {
        return reinterpret_cast<TargetProcess::Address>(*addr);
    } else {
        Diagnostics::abort("failed to allocate memory for section");
    }
}

void HotReloader::resolve_symbol_use(BinModule &mod, const LoadedFunc &loaded_func, const BinSymbolUse &use) {
    TargetProcess::Address use_addr = 0;
    if (use.section == BinSectionKind::TEXT) {
        use_addr = loaded_func.text_addr + use.address;
    } else if (use.section == BinSectionKind::DATA) {
        use_addr = loaded_func.data_addr + use.address;
    }

    const BinSymbolDef &def = mod.symbol_defs[use.symbol_index];
    TargetProcess::Address def_addr = 0;

    if (def.name == "addr_table") {
        def_addr = addr_table_ptr;
    } else if (def.kind == BinSymbolKind::DATA_LABEL) {
        def_addr = loaded_func.data_addr + def.offset;
    }

    if (use.kind == BinSymbolUseKind::REL32) {
        std::ptrdiff_t offset = def_addr - (use_addr + 4);
        mod.text.seek(use.address);
        mod.text.write_i32(offset);
    } else if (use.kind == BinSymbolUseKind::ABS64) {
        if (use.section == BinSectionKind::TEXT) {
            mod.text.seek(use.address);
            mod.text.write_u64(def_addr);
        } else if (use.section == BinSectionKind::DATA) {
            mod.data.seek(use.address);
            mod.data.write_u64(def_addr);
        }
    }
}

void HotReloader::write_section(TargetProcess::Address address, const WriteBuffer &buffer) {
    if (buffer.get_size() == 0) {
        return;
    }

    bool result = process->write_memory(address, buffer.get_data().data(), buffer.get_size());

    if (!result) {
        Diagnostics::abort("failed to write section");
    }
}

void HotReloader::update_func_addr(lang::sir::FuncDef &func_def, unsigned index, TargetProcess::Address new_addr) {
    TargetProcess::Address item_addr = addr_table_ptr + addr_table.compute_offset(index);
    bool result = process->write_memory(item_addr, &new_addr, sizeof(TargetProcess::Address));

    if (result) {
        Diagnostics::log("updated function '" + Diagnostics::symbol_to_string(&func_def) + "'");
    } else {
        Diagnostics::abort("failed to write function address to address table");
    }
}

} // namespace hot_reloader

} // namespace banjo

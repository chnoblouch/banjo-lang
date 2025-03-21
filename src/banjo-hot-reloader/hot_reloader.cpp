#include "hot_reloader.hpp"

#include "banjo/ssa_gen/name_mangling.hpp"
#include "banjo/utils/platform.hpp"
#include "file_watcher.hpp"
#include "jit_compiler.hpp"
#include "target_process.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

namespace banjo {

namespace hot_reloader {

HotReloader::HotReloader() {
    std::string os;

#if OS_WINDOWS
    os = "windows";
#elif OS_LINUX
    os = "linux";
#endif

    log("platform: x86_64-" + os);
}

void HotReloader::run(const std::string &executable, const std::filesystem::path &src_path) {
    process = TargetProcess::spawn(executable);

    if (process) {
        log("executable loaded");
    } else {
        abort("failed to load executable");
    }

    std::optional<TargetProcess::Address> pointer = process->find_section(".bnjatbl");

    if (pointer) {
        addr_table_ptr = *pointer;
        log("found address table in target process");
    } else {
        abort("failed to find address table in target process");
    }

    TargetProcess::Address target_addr = addr_table_ptr;

    std::uint32_t num_symbols;
    if (process->read_memory(target_addr, &num_symbols, 4)) {
        target_addr += 4;
    } else {
        abort("failed to read number of symbols");
    }

    for (unsigned i = 0; i < num_symbols; i++) {
        std::uint32_t symbol_length;
        if (process->read_memory(target_addr, &symbol_length, 4)) {
            target_addr += 4;
        } else {
            abort("failed to read symbol length");
        }

        std::string symbol_name(symbol_length, '\0');
        if (process->read_memory(target_addr, symbol_name.data(), symbol_length)) {
            target_addr += symbol_length;
        } else {
            abort("failed to read symbol name");
        }

        addr_table.append(symbol_name);
    }

    log("address table layout loaded (" + std::to_string(num_symbols) + " symbols)");

    std::string canonical_src_path = std::filesystem::canonical(src_path).string();

    std::optional<FileWatcher> watcher = FileWatcher::open(src_path);
    if (watcher) {
        log("watching directory '" + canonical_src_path + "'");
    } else {
        abort("cannot watch directory '" + canonical_src_path + "'");
    }

    process->poll();

    while (!process->is_exited()) {
        std::optional<std::vector<std::filesystem::path>> paths = watcher->poll(25);
        if (!paths) {
            abort("failed to poll directory watcher");
        }

        // FIXME: Reading the changed file right away might fail if it is
        // still opened by the process that modified it.

        for (const std::filesystem::path &path : *paths) {
            if (path.extension() == ".bnj" && has_changed(path)) {
                reload_file(path);
            }
        }

        process->poll();
    }

    watcher->close();
    process->close();
    log("process exited");
}

bool HotReloader::has_changed(const std::filesystem::path &file_path) {
    std::string path_string = file_path.string();

    auto prev_content_iter = file_contents.find(path_string);
    std::vector<char> *prev_content = prev_content_iter == file_contents.end() ? nullptr : &prev_content_iter->second;

    std::vector<char> curr_content;

    std::ifstream stream(path_string);
    if (!stream.good()) {
        return false;
    }

    stream.seekg(0, std::ios::end);

    std::ios::pos_type size = stream.tellg();
    if (prev_content && size != prev_content->size()) {
        return true;
    }

    if (size != 0) {
        curr_content.resize(size);
        stream.seekg(0, std::ios::beg);
        stream.read(curr_content.data(), size);
    }

    bool changed = prev_content ? (curr_content != *prev_content) : true;
    file_contents[path_string] = curr_content;
    return changed;
}

void HotReloader::reload_file(const std::filesystem::path &file_path) {
    log("reloading file '" + file_path.string() + "'...");

    JITCompiler compiler(lang::Config::instance(), addr_table);
    if (!compiler.build_ir()) {
        log("failed to reload file");
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
        abort("failed to allocate memory for section");
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
        abort("failed to write section");
    }
}

void HotReloader::update_func_addr(lang::sir::FuncDef &func_def, unsigned index, TargetProcess::Address new_addr) {
    TargetProcess::Address item_addr = addr_table_ptr + addr_table.compute_offset(index);
    bool result = process->write_memory(item_addr, &new_addr, sizeof(TargetProcess::Address));

    if (result) {
        log("updated function '" + symbol_to_string(&func_def) + "'");
    } else {
        abort("failed to write function address to address table");
    }
}

void HotReloader::log(const std::string &message) {
    std::cout << "(hot reloader) " << message << "\n";
}

[[noreturn]] void HotReloader::abort(const std::string &message) {
    std::cerr << "(hot reloader) error: " << message << "\n";
    std::exit(1);
}

std::string HotReloader::symbol_to_string(lang::sir::Symbol symbol) {
    std::string result;

    lang::sir::Symbol parent = symbol.get_parent();
    if (parent) {
        result += symbol_to_string(parent) + '.';
    }

    if (auto mod = symbol.match<lang::sir::Module>()) {
        result += mod->path.to_string();
    } else if (auto func_def = symbol.match<lang::sir::FuncDef>()) {
        result += func_def->ident.value;
    } else if (auto struct_def = symbol.match<lang::sir::StructDef>()) {
        result += struct_def->ident.value;
    } else if (auto union_def = symbol.match<lang::sir::UnionDef>()) {
        result += union_def->ident.value;
    }

    return result;
}

} // namespace hot_reloader

} // namespace banjo

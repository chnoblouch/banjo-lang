#ifndef HOT_RELOADER_H
#define HOT_RELOADER_H

#include "banjo/emit/binary_module.hpp"
#include "banjo/ssa/addr_table.hpp"
#include "banjo/sir/sir.hpp"
#include "target_process.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace hot_reloader {

class HotReloader {

private:
    struct LoadedFunc {
        TargetProcess::Address text_addr;
        TargetProcess::Size text_size;
        TargetProcess::Address data_addr;
        TargetProcess::Size data_size;
    };

    std::optional<TargetProcess> process;
    TargetProcess::Address addr_table_ptr;
    ssa::AddrTable addr_table;
    std::unordered_map<std::string, std::vector<char>> file_contents;

public:
    HotReloader();
    void run(const std::string &executable, const std::filesystem::path &src_path);

private:
    bool has_changed(const std::filesystem::path &file_path);
    void reload_file(const std::filesystem::path &file_path);
    void collect_funcs(lang::sir::DeclBlock &block, std::vector<lang::sir::FuncDef *> &out_funcs);
    LoadedFunc load_func(BinModule &mod);
    TargetProcess::Address alloc_section(TargetProcess::Size size, TargetProcess::MemoryProtection protection);
    void resolve_symbol_use(BinModule &mod, const LoadedFunc &loaded_func, const BinSymbolUse &use);
    void write_section(TargetProcess::Address address, const WriteBuffer &buffer);
    void update_func_addr(lang::sir::FuncDef &func_def, unsigned index, TargetProcess::Address new_addr);

    static void log(const std::string &message);
    [[noreturn]] static void abort(const std::string &message);
    static std::string symbol_to_string(lang::sir::Symbol symbol);
};

} // namespace hot_reloader

} // namespace banjo

#endif

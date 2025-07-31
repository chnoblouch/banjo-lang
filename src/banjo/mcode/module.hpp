#ifndef BANJO_MCODE_MODULE_H
#define BANJO_MCODE_MODULE_H

#include "banjo/mcode/function.hpp"
#include "banjo/mcode/global.hpp"

#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace banjo {

namespace mcode {

struct AddrTable {
    std::vector<std::string> entries;
};

class Module {

private:
    std::vector<Function *> functions;
    std::vector<Global> globals;
    std::unordered_set<std::string> external_symbols;
    std::unordered_set<std::string> global_symbols;
    std::vector<std::string> dll_exports;
    std::optional<AddrTable> addr_table;
    std::any target_data;

    int last_float_label_id = 0;

public:
    Module() {}
    Module(const Module &) = delete;
    Module(Module &&) noexcept = default;
    ~Module();

    Module &operator=(const Module &) = delete;
    Module &operator=(Module &&) = default;

    std::vector<Function *> &get_functions() { return functions; }
    std::vector<Global> &get_globals() { return globals; }
    std::unordered_set<std::string> &get_external_symbols() { return external_symbols; }
    std::unordered_set<std::string> &get_global_symbols() { return global_symbols; }
    std::vector<std::string> &get_dll_exports() { return dll_exports; }
    std::optional<AddrTable> &get_addr_table() { return addr_table; }
    const std::any &get_target_data() { return target_data; }

    void add(Function *function) { functions.push_back(function); }
    void add(Global global) { globals.push_back(std::move(global)); }
    void add_external_symbol(std::string external_symbol) { external_symbols.insert(std::move(external_symbol)); }
    void add_global_symbol(std::string global_symbol) { global_symbols.insert(std::move(global_symbol)); }
    void add_dll_export(std::string dll_export) { dll_exports.push_back(std::move(dll_export)); }
    void set_addr_table(AddrTable addr_table) { this->addr_table = std::move(addr_table); }
    void set_target_data(std::any target_data) { this->target_data = std::move(target_data); }

    std::string next_float_label();
};

} // namespace mcode

} // namespace banjo

#endif

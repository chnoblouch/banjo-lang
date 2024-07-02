#ifndef MCODE_MODULE_H
#define MCODE_MODULE_H

#include "mcode/function.hpp"
#include "mcode/global.hpp"

#include <unordered_set>
#include <vector>

namespace banjo {

namespace mcode {

class Module {

private:
    std::vector<Function *> functions;
    std::vector<Global> globals;
    std::vector<std::string> external_symbols;
    std::unordered_set<std::string> global_symbols;
    std::vector<std::string> dll_exports;

    int last_float_label_id = 0;

public:
    Module() {}
    Module(const Module &) = delete;
    Module(Module &&) = default;
    ~Module();

    Module &operator=(const Module &) = delete;
    Module &operator=(Module &&) = default;

    std::vector<Function *> &get_functions() { return functions; }
    std::vector<Global> &get_globals() { return globals; }
    std::vector<std::string> &get_external_symbols() { return external_symbols; }
    std::unordered_set<std::string> &get_global_symbols() { return global_symbols; }
    std::vector<std::string> &get_dll_exports() { return dll_exports; }

    void add(Function *function) { functions.push_back(function); }
    void add(Global global) { globals.push_back(global); }
    void add_external_symbol(std::string external_symbol) { external_symbols.push_back(external_symbol); }
    void add_global_symbol(std::string global_symbol) { global_symbols.insert(global_symbol); }
    void add_dll_export(std::string dll_export) { dll_exports.push_back(dll_export); }

    std::string next_float_label();
};

} // namespace mcode

} // namespace banjo

#endif

#ifndef IR_MODULE_H
#define IR_MODULE_H

#include "banjo/ssa/addr_table.hpp"
#include "banjo/ssa/function.hpp"
#include "banjo/ssa/function_decl.hpp"
#include "banjo/ssa/global.hpp"
#include "banjo/ssa/structure.hpp"

#include <vector>

namespace banjo {

namespace ssa {

class Module {

private:
    std::vector<Function *> functions;
    std::vector<Global *> globals;
    std::vector<ssa::Structure *> structures;
    std::vector<FunctionDecl *> external_functions;
    std::vector<GlobalDecl *> external_globals;
    std::vector<std::string> dll_exports;
    std::optional<AddrTable> addr_table;

    std::vector<std::vector<ssa::Type>> tuple_types;

public:
    Module() {}
    Module(const Module &) = delete;
    Module(Module &&) = default;
    ~Module();

    Module &operator=(const Module &) = delete;
    Module &operator=(Module &&) = default;

    std::vector<Function *> &get_functions() { return functions; }
    std::vector<Global *> &get_globals() { return globals; }
    std::vector<ssa::Structure *> &get_structures() { return structures; }
    std::vector<FunctionDecl *> &get_external_functions() { return external_functions; }
    std::vector<GlobalDecl *> &get_external_globals() { return external_globals; }
    std::vector<std::string> &get_dll_exports() { return dll_exports; }
    std::optional<AddrTable> &get_addr_table() { return addr_table; }

    Function *get_function(const std::string &name);
    Structure *get_structure(const std::string &name);

    void set_functions(std::vector<Function *> functions) { this->functions = std::move(functions); }

    void set_external_functions(std::vector<FunctionDecl *> external_functions) {
        this->external_functions = std::move(external_functions);
    }

    void add(Function *function) { functions.push_back(function); }
    void add(Global *global) { globals.push_back(global); }
    void add(Structure *structure) { structures.push_back(structure); }
    void add(FunctionDecl *external_function) { external_functions.push_back(external_function); }
    void add(GlobalDecl *external_global) { external_globals.push_back(external_global); }
    void add_dll_export(std::string dll_export) { dll_exports.push_back(std::move(dll_export)); }
    void set_addr_table(AddrTable addr_table) { this->addr_table = std::move(addr_table); }

    void forget_pointers();
};

} // namespace ssa

} // namespace banjo

#endif

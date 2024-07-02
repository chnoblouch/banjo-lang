#ifndef IR_MODULE_H
#define IR_MODULE_H

#include "banjo/ir/function.hpp"
#include "banjo/ir/function_decl.hpp"
#include "banjo/ir/global.hpp"
#include "banjo/ir/structure.hpp"

#include <vector>

namespace banjo {

namespace ir {

class Module {

private:
    std::vector<Function *> functions;
    std::vector<Global> globals;
    std::vector<ir::Structure *> structures;
    std::vector<FunctionDecl> external_functions;
    std::vector<GlobalDecl> external_globals;
    std::vector<std::string> dll_exports;

    std::vector<std::vector<ir::Type>> tuple_types;

public:
    Module() {}
    Module(const Module &) = delete;
    Module(Module &&) = default;
    ~Module();

    Module &operator=(const Module &) = delete;
    Module &operator=(Module &&) = default;

    std::vector<Function *> &get_functions() { return functions; }
    std::vector<Global> &get_globals() { return globals; }
    std::vector<ir::Structure *> &get_structures() { return structures; }
    std::vector<FunctionDecl> &get_external_functions() { return external_functions; }
    std::vector<GlobalDecl> &get_external_globals() { return external_globals; }
    std::vector<std::string> &get_dll_exports() { return dll_exports; }

    Function *get_function(const std::string &name);
    Structure *get_structure(const std::string &name);

    void set_functions(std::vector<Function *> functions) { this->functions = std::move(functions); }

    void add(Function *function) { functions.push_back(function); }
    void add(Global global) { globals.push_back(std::move(global)); }
    void add(Structure *structure) { structures.push_back(structure); }
    void add(FunctionDecl external_function) { external_functions.push_back(std::move(external_function)); }
    void add(GlobalDecl external_global) { external_globals.push_back(std::move(external_global)); }
    void add_dll_export(std::string dll_export) { dll_exports.push_back(std::move(dll_export)); }

    void forget_pointers();
};

} // namespace ir

} // namespace banjo

#endif

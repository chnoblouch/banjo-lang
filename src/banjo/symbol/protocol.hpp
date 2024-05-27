#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "ir/structure.hpp"
#include "symbol/data_type.hpp"
#include "symbol/symbol.hpp"

#include <string>
#include <utility>
#include <vector>

namespace lang {

struct FunctionSignature {
    std::string name;
    FunctionType type;
    unsigned index;
};

class Protocol : public Symbol {

private:
    std::vector<FunctionSignature> func_signatures;
    ir::Structure *ir_vtable_struct = nullptr;

public:
    Protocol() : Symbol(nullptr, "") {}
    Protocol(ASTNode *node, std::string name) : Symbol(node, std::move(name)) {}

    const std::vector<FunctionSignature> &get_func_signatures() const { return func_signatures; }
    FunctionSignature *get_func_signature(const std::string &name);
    ir::Structure *get_ir_vtable_struct() { return ir_vtable_struct; }

    void add_func_signature(std::string name, FunctionType type);
    void set_ir_vtable_struct(ir::Structure *ir_vtable_struct) { this->ir_vtable_struct = ir_vtable_struct; }
};

} // namespace lang

#endif

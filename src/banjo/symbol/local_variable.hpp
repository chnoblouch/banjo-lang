#ifndef LOCAL_VARIABLE_H
#define LOCAL_VARIABLE_H

#include "ir/virtual_register.hpp"
#include "symbol/variable.hpp"

namespace lang {

class LocalVariable : public Variable {

private:
    ir::VirtualRegister virtual_reg = -1;

public:
    LocalVariable(ASTNode *node, DataType *data_type, std::string name);

    ir::VirtualRegister get_virtual_reg() { return virtual_reg; }
    void set_virtual_reg(ir::VirtualRegister virtual_reg) { this->virtual_reg = virtual_reg; }

    ir_builder::StoredValue as_ir_value(ir_builder::IRBuilderContext &context) override final;
};

} // namespace lang

#endif

#ifndef PARAMETER_H
#define PARAMETER_H

#include "ir/virtual_register.hpp"
#include "symbol/variable.hpp"

namespace banjo {

namespace lang {

class Parameter : public Variable {

private:
    ir::VirtualRegister virtual_reg = -1;
    bool pass_by_ref = false;

public:
    Parameter(ASTNode *node, DataType *data_type, std::string name);

    ir::VirtualRegister get_virtual_reg() { return virtual_reg; }
    bool is_pass_by_ref() { return pass_by_ref; }

    void set_virtual_reg(ir::VirtualRegister virtual_reg) { this->virtual_reg = virtual_reg; }
    void set_pass_by_ref(bool pass_by_ref) { this->pass_by_ref = pass_by_ref; }

    ir_builder::StoredValue as_ir_value(ir_builder::IRBuilderContext &context) override final;
};

} // namespace lang

} // namespace banjo

#endif

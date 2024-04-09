#ifndef PARAMETER_H
#define PARAMETER_H

#include "symbol/variable.hpp"

namespace lang {

class Parameter : public Variable {

private:
    long virtual_reg_id = -1;
    bool pass_by_ref = false;

public:
    Parameter(ASTNode *node, DataType *data_type, std::string name);

    long get_virtual_reg_id() { return virtual_reg_id; }
    bool is_pass_by_ref() { return pass_by_ref; }

    void set_virtual_reg_id(long virtual_reg_id) { this->virtual_reg_id = virtual_reg_id; }
    void set_pass_by_ref(bool pass_by_ref) { this->pass_by_ref = pass_by_ref; }

    ir::Operand as_ir_operand(ir_builder::IRBuilderContext &context);
    bool is_ir_operand_reference() { return true; }
};

} // namespace lang

#endif

#ifndef LOCAL_VARIABLE_H
#define LOCAL_VARIABLE_H

#include "symbol/variable.hpp"

namespace lang {

class LocalVariable : public Variable {

private:
    long virtual_reg_id = -1;

public:
    LocalVariable(ASTNode *node, DataType *data_type, std::string name);

    long get_virtual_reg_id() { return virtual_reg_id; }
    void set_virtual_reg_id(long virtual_reg_id) { this->virtual_reg_id = virtual_reg_id; }

    ir::Operand as_ir_operand(ir_builder::IRBuilderContext &context);
    bool is_ir_operand_reference() { return true; }
};

} // namespace lang

#endif

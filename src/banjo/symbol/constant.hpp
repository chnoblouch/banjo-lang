#ifndef CONSTANT_VARIABLE_H
#define CONSTANT_VARIABLE_H

#include "symbol/variable.hpp"

namespace lang {

class ASTNode;
class ASTModule;

class Constant : public Variable {

private:
    ASTModule *module_;
    ASTNode *value;

public:
    Constant();
    Constant(ASTNode *node, std::string name, ASTModule *module_);

    ASTModule *get_module() { return module_; }
    ASTNode *get_value() { return value; }

    void set_value(ASTNode *value) { this->value = value; }

    ir_builder::StoredValue as_ir_value(ir_builder::IRBuilderContext &context) override final;
};

} // namespace lang

#endif

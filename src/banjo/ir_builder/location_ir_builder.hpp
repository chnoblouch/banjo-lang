#ifndef IR_BUILDER_LOCATION_IR_BUILDER_H
#define IR_BUILDER_LOCATION_IR_BUILDER_H

#include "ir_builder/ir_builder.hpp"
#include "symbol/data_type.hpp"
#include "symbol/function.hpp"
#include "symbol/location.hpp"
#include "symbol/module_path.hpp"
#include "symbol/union.hpp"
#include "symbol/variable.hpp"

#include <optional>
#include <vector>

namespace ir_builder {

class LocationIRBuilder : public IRBuilder {

private:
    std::optional<std::vector<lang::DataType *>> params;

    lang::ModulePath mod_path;
    lang::Variable *var = nullptr;
    lang::Function *func = nullptr;

    ir::VirtualRegister dest{-1};
    ir::Operand operand;
    bool operand_ref;

    ir::Operand self_operand;
    bool self_ptr;
    lang::DataType *self_type;

public:
    LocationIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}

    LocationIRBuilder(IRBuilderContext &context, lang::ASTNode *node, std::vector<lang::DataType *> params)
      : IRBuilder(context, node),
        params{params} {}

    ir::Value build(bool return_value);
    ir::Value build_location(const lang::Location &location, bool return_value);
    lang::Function *get_lang_func() { return func; }
    bool is_operand_ref() { return operand_ref; }
    ir::VirtualRegister get_dest() { return dest; }

    ir::Operand get_self_operand() { return self_operand; }
    bool is_self_ptr() { return self_ptr; }
    lang::DataType *get_self_type() { return self_type; }

private:
    void build(const lang::Location &location);
    void build_root(const lang::LocationElement &element);
    void build_var(lang::Variable *var);
    bool is_captured_var(lang::Variable *var);
    void build_captured_var(lang::Variable *var);
    void build_element(const lang::LocationElement &element, lang::DataType *previous_type);
    void build_struct_field_access(lang::StructField *field, lang::Structure *struct_);
    void build_union_case_field_access(lang::UnionCaseField *field, lang::UnionCase *case_);
    void build_direct_method_call(lang::Function *method);
    void build_ptr_field_access(lang::StructField *field, lang::Structure *struct_);
    void build_ptr_method_call(lang::Function *method);
};

} // namespace ir_builder

#endif

#ifndef IR_BUILDER_FUNC_CALL_IR_BUILDER_H
#define IR_BUILDER_FUNC_CALL_IR_BUILDER_H

#include "ir_builder/ir_builder.hpp"
#include "ir_builder/storage.hpp"
#include "symbol/data_type.hpp"

#include <optional>
#include <vector>

namespace ir_builder {

class LocationIRBuilder;

class FuncCallIRBuilder : public IRBuilder {

private:
    lang::FunctionType type;

public:
    FuncCallIRBuilder(IRBuilderContext &context, lang::ASTNode *node) : IRBuilder(context, node) {}

    ir::VirtualRegister build(StorageHints hints, bool use_result);
    lang::FunctionType get_type() { return type; }

private:
    std::vector<lang::DataType *> infer_params();
    ir::Operand get_self_operand(LocationIRBuilder &location_builder);
};

} // namespace ir_builder

#endif

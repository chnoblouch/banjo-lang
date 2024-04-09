#ifndef CONVERT_IR_BUILDER_H
#define CONVERT_IR_BUILDER_H

#include "ir_builder/ir_builder_context.hpp"
#include "symbol/data_type.hpp"

namespace ir_builder {

namespace Conversion {

ir::Value build(IRBuilderContext &context, ir::Value &value, lang::DataType *from, lang::DataType *to);

} // namespace Conversion

} // namespace ir_builder

#endif

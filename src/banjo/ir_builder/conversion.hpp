#ifndef CONVERT_IR_BUILDER_H
#define CONVERT_IR_BUILDER_H

#include "banjo/ir_builder/ir_builder_context.hpp"
#include "banjo/symbol/data_type.hpp"

namespace banjo {

namespace ir_builder {

namespace Conversion {

ir::Value build(IRBuilderContext &context, ir::Value &value, lang::DataType *from, lang::DataType *to);

} // namespace Conversion

} // namespace ir_builder

} // namespace banjo

#endif

#ifndef BANJO_SSA_GEN_CALL_SSA_BUILDER_H
#define BANJO_SSA_GEN_CALL_SSA_BUILDER_H

#include "banjo/ssa/operand.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"
#include "banjo/ssa_gen/stored_value.hpp"

#include <vector>

namespace banjo::lang {

class CallSSABuilder {

private:
    SSAGeneratorContext &ctx;
    ssa::Type ssa_return_type;
    const StorageHints &hints;

    ReturnMethod return_method;
    std::vector<ssa::Operand> ssa_operands;
    StoredValue return_value_ptr;

public:
    CallSSABuilder(SSAGeneratorContext &ctx, ssa::Type ssa_return_type, const StorageHints &hints);
    CallSSABuilder &set_callee(const ssa::Value &ssa_callee);
    CallSSABuilder &add_arg(StoredValue value);
    StoredValue generate();
};

} // namespace banjo::lang

#endif

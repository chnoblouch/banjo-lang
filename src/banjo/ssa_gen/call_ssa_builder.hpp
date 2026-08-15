#ifndef BANJO_SSA_GEN_CALL_SSA_BUILDER_H
#define BANJO_SSA_GEN_CALL_SSA_BUILDER_H

#include "banjo/ssa/operand.hpp"
#include "banjo/ssa_gen/ssa_generator_context.hpp"
#include "banjo/ssa_gen/storage_hints.hpp"
#include "banjo/ssa_gen/stored_value.hpp"

#include <optional>
#include <vector>

namespace banjo {

class CallSSABuilder {

private:
    SSAGeneratorContext &ctx;
    ssa::Type ssa_return_type;
    const StorageHints &hints;

    ReturnMethod return_method;
    std::vector<ssa::Operand> ssa_operands;
    StoredValue return_value_ptr;
    std::optional<unsigned> first_variadic_index;

public:
    CallSSABuilder(SSAGeneratorContext &ctx, ssa::Type ssa_return_type, const StorageHints &hints);
    CallSSABuilder &set_callee(const ssa::Value &ssa_callee);
    CallSSABuilder &add_arg(StoredValue value);
    CallSSABuilder &make_variadic(unsigned first_variadic_index);
    StoredValue generate();
};

} // namespace banjo

#endif

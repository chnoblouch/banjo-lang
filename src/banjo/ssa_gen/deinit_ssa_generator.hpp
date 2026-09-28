#ifndef BANJO_SSA_GENERATOR_DEINIT_SSA_GENERATOR_H
#define BANJO_SSA_GENERATOR_DEINIT_SSA_GENERATOR_H

#include "banjo/ssa_gen/ssa_generator_context.hpp"

namespace banjo {

class DeinitSSAGenerator {

private:
    SSAGeneratorContext &ctx;

public:
    DeinitSSAGenerator(SSAGeneratorContext &ctx);

    void generate_resource_flag_slot(const sir::Resource &resource, ssa::Value initial_value);
    void generate_deferred_deinits();
    void generate_deinit(const sir::Resource &resource, sir::Symbol symbol);
    void generate_deinit(const sir::Resource &resource, ssa::Value ssa_ptr);
    void generate_deinit_call(const sir::Resource &resource, ssa::Value ssa_ptr);
};

} // namespace banjo

#endif

#ifndef SSA_GEN_STORAGE_HINTS_H
#define SSA_GEN_STORAGE_HINTS_H

#include "banjo/ssa/operand.hpp"
#include "banjo/ssa/virtual_register.hpp"

#include <optional>

namespace banjo {

namespace lang {

struct StorageHints {
    std::optional<ssa::Value> dst;
    bool is_prefer_reference;
    bool is_unused;

    static StorageHints none() { return {{}, false, false}; }
    static StorageHints into(ssa::Value dst) { return {dst, false, false}; }
    static StorageHints into(ssa::VirtualRegister dst) { return {ssa::Value::from_register(dst), false, false}; }
    static StorageHints prefer_reference() { return {{}, true, false}; }
    static StorageHints unused() { return {{}, false, true}; }
};

} // namespace lang

} // namespace banjo

#endif

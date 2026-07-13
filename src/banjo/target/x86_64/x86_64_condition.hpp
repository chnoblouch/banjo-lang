#ifndef BANJO_TARGET_X86_64_CONDITION_H
#define BANJO_TARGET_X86_64_CONDITION_H

namespace banjo::target {

enum class X8664Condition {
    E,
    NE,
    A,
    AE,
    B,
    BE,
    G,
    GE,
    L,
    LE,
};

} // namespace banjo::target

#endif

#ifndef BANJO_TARGET_AARCH64_CONDITION_H
#define BANJO_TARGET_AARCH64_CONDITION_H

namespace banjo::target {

enum class AArch64Condition {
    EQ,
    NE,
    HS,
    LO,
    HI,
    LS,
    GE,
    LT,
    GT,
    LE,
};

} // namespace banjo::target

#endif

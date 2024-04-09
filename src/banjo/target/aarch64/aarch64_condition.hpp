#ifndef AARCH64_CONDITION_H
#define AARCH64_CONDITION_H

namespace target {

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

} // namespace target

#endif

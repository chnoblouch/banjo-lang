#ifndef IR_CALLING_CONV_H
#define IR_CALLING_CONV_H

namespace banjo {

namespace ssa {

enum class CallingConv {
    NONE,
    X86_64_SYS_V_ABI,
    X86_64_MS_ABI,
    AARCH64_AAPCS,
};

} // namespace ssa

} // namespace banjo

#endif

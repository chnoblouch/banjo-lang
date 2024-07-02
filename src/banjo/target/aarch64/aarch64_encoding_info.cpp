#include "aarch64_encoding_info.hpp"

#include <cassert>

namespace banjo {

namespace target {

bool AArch64EncodingInfo::is_addr_offset_encodable(int offset, int size) {
    switch (size) {
        case 1: return is_addr_8_offset_encodable(offset);
        case 2: return is_addr_16_offset_encodable(offset);
        case 4: return is_addr_32_offset_encodable(offset);
        case 8: return is_addr_64_offset_encodable(offset);
        default: assert(false); return false;
    }
}

bool AArch64EncodingInfo::is_addr_8_offset_encodable(int offset) {
    // https://developer.arm.com/documentation/ddi0596/2020-12/Base-Instructions/LDRB--immediate---Load-Register-Byte--immediate--

    // Offsets can be in the range [-256; 255] (pre-index) or [0; 4095] (post-index).
    return offset >= -256 && offset <= 4095;
}

bool AArch64EncodingInfo::is_addr_16_offset_encodable(int offset) {
    // https://developer.arm.com/documentation/ddi0596/2020-12/Base-Instructions/LDRH--immediate---Load-Register-Halfword--immediate--

    if (offset >= -256 && offset <= 255) {
        // Offsets in the range [-256; 255] can be encoded in the post-index mode.
        return true;
    }

    // Other offsets must be a multiple of 2 in the range [0, 8190]
    return offset >= 0 && offset <= 8190 && offset % 2 == 0;
}

bool AArch64EncodingInfo::is_addr_32_offset_encodable(int offset) {
    // https://developer.arm.com/documentation/ddi0596/2020-12/Base-Instructions/LDR--immediate---Load-Register--immediate--

    if (offset >= -4096 && offset <= 4095) {
        // Offsets in the range [-4096; 4095] can be encoded in the post-index mode.
        return true;
    }

    // Other offsets must be a multiple of 4 in the range [0; 32760].
    return offset >= 0 && offset <= 16380 && offset % 4 == 0;
}

bool AArch64EncodingInfo::is_addr_64_offset_encodable(int offset) {
    // https://developer.arm.com/documentation/ddi0596/2020-12/Base-Instructions/LDR--immediate---Load-Register--immediate--

    if (offset >= -256 && offset <= 255) {
        // Offsets in the range [-256; 256] can be encoded in the post-index mode.
        return true;
    }

    // Other offsets must be a multiple of 8 in the range [0; 32760].
    return offset >= 0 && offset <= 32760 && offset % 8 == 0;
}

} // namespace target

} // namespace banjo

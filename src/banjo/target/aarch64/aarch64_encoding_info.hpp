#ifndef AARCH64_ENCODING_INFO_H
#define AARCH64_ENCODING_INFO_H

namespace banjo {

namespace target {

namespace AArch64EncodingInfo {

bool is_addr_offset_encodable(int offset, int size);
bool is_addr_8_offset_encodable(int offset);
bool is_addr_16_offset_encodable(int offset);
bool is_addr_32_offset_encodable(int offset);
bool is_addr_64_offset_encodable(int offset);

} // namespace AArch64EncodingInfo

} // namespace target

} // namespace banjo

#endif

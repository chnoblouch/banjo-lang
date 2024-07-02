#ifndef BINARY_EMITTER_H
#define BINARY_EMITTER_H

#include "emit/emitter.hpp"
#include <cstdint>

namespace banjo {

namespace codegen {

class BinaryEmitter : public Emitter {

public:
    BinaryEmitter(mcode::Module &module, std::ostream &stream) : Emitter(module, stream) {}

protected:
    // FIXME: all of this won't work on big-endian machines
    void emit_u8(std::uint8_t u8) { stream.write((char *)&u8, 1); }
    void emit_u16(std::uint16_t u16) { stream.write((char *)&u16, 2); }
    void emit_u32(std::uint32_t u32) { stream.write((char *)&u32, 4); }
    void emit_u64(std::uint64_t u64) { stream.write((char *)&u64, 8); }
    void emit_i8(std::int8_t i8) { stream.write((char *)&i8, 1); }
    void emit_i16(std::int16_t i16) { stream.write((char *)&i16, 2); }
    void emit_i32(std::int32_t i32) { stream.write((char *)&i32, 4); }
    void emit_i64(std::int64_t i64) { stream.write((char *)&i64, 8); }
    std::size_t tell() { return stream.tellp(); }
    void seek(std::size_t pos) { stream.seekp(pos, std::ios::beg); }
};

} // namespace codegen

} // namespace banjo

#endif

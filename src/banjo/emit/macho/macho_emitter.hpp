#ifndef BANJO_EMIT_MACHO_MACHO_EMITTER_H
#define BANJO_EMIT_MACHO_MACHO_EMITTER_H

#include "banjo/emit/binary_emitter.hpp"
#include "banjo/emit/macho/macho_format.hpp"
#include "banjo/target/target_description.hpp"

namespace banjo {
namespace codegen {

class MachOEmitter : public codegen::BinaryEmitter {

private:
    target::TargetDescription target;

    std::vector<std::size_t> markers;
    unsigned marker_index = 0;

public:
    MachOEmitter(mcode::Module &module, std::ostream &stream, target::TargetDescription target)
      : BinaryEmitter(module, stream),
        target(target) {}

    void generate();

private:
    void emit_file(const MachOFile &file);

    void emit_headers(const MachOFile &file);
    void emit_file_header(const MachOFile &file);
    void emit_segment_header(const MachOSegment &segment);
    void emit_symtab_header(const MachOSymtabCommand &command);
    void emit_dysymtab_header(const MachODysymtabCommand &command);
    void emit_build_version_header(const MachOBuildVersionCommand &command);

    void emit_data(const MachOFile &file);
    void emit_segment_data(const MachOSegment &segment);
    void emit_symtab_data(const MachOSymtabCommand &command);

    void emit_placeholder_u32();
    void emit_placeholder_u64();
    void patch_u32(std::size_t position, std::uint32_t value);
    void patch_u64(std::size_t position, std::uint64_t value);
    void emit_name_padded(const std::string &name);

    void push_marker();
    std::size_t consume_marker();
};

} // namespace codegen
} // namespace banjo

#endif

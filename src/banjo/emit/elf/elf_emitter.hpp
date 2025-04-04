#ifndef ELF_EMITTER_H
#define ELF_EMITTER_H

#include "banjo/emit/binary_emitter.hpp"
#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/elf/elf_format.hpp"
#include "banjo/target/target_description.hpp"

namespace banjo {

namespace codegen {

class ELFEmitter : public BinaryEmitter {

private:
    struct SectionPointerPositions {
        std::size_t data;
    };

    target::TargetDescription target;

public:
    ELFEmitter(mcode::Module &module, std::ostream &stream, target::TargetDescription target)
      : BinaryEmitter(module, stream),
        target(target) {}

    void generate();

private:
    BinModule generate_bin_mod();
    ELFFile build_file(BinModule &bin_mod);

    void emit_file(const ELFFile &file);
    void emit_header(const ELFFile &file);
    void emit_sections(const std::vector<ELFSection> &sections);
    void emit_section_header(const ELFSection &section, SectionPointerPositions &pointer_positions);
    void emit_section_data(const ELFSection::Data &data, std::size_t pointer_pos);
};

} // namespace codegen

} // namespace banjo

#endif

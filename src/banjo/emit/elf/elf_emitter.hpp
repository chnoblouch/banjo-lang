#ifndef ELF_EMITTER_H
#define ELF_EMITTER_H

#include "emit/binary_emitter.hpp"
#include "emit/elf/elf_format.hpp"

namespace codegen {

class ELFEmitter : public BinaryEmitter {

private:
    struct SectionPointerPositions {
        std::size_t data;
    };

public:
    ELFEmitter(mcode::Module &module, std::ostream &stream) : BinaryEmitter(module, stream) {}
    void generate();

private:
    void emit_file(const ELFFile &file);
    void emit_header(const ELFFile &file);
    void emit_sections(const std::vector<ELFSection> &sections);
    void emit_section_header(const ELFSection &section, SectionPointerPositions &pointer_positions);
    void emit_section_data(const ELFSection::Data &data, std::size_t pointer_pos);
};

} // namespace codegen

#endif

#ifndef BANJO_EMIT_PE_EMITTER_H
#define BANJO_EMIT_PE_EMITTER_H

#include "banjo/emit/binary_emitter.hpp"
#include "banjo/emit/pe/pe_format.hpp"

namespace banjo {

namespace codegen {

class PEEmitter : public BinaryEmitter {

private:
    struct SectionPointerPositions {
        std::size_t data;
        std::size_t relocations;
    };

public:
    PEEmitter(mcode::Module &module, std::ostream &stream) : BinaryEmitter(module, stream) {}
    void generate();

private:
    void emit_file(const PEFile &file);
    void emit_header(const PEFile &file);
    void emit_sections(const std::vector<PESection> &sections);
    void emit_section_header(const PESection &section, SectionPointerPositions &pointer_positions);
    void emit_section_data(const std::vector<std::uint8_t> &data, std::size_t pointer_pos);
    void emit_section_relocations(const std::vector<PERelocation> &relocations, std::size_t pointer_pos);
    void emit_symbol_table(const std::vector<PESymbol> &symbols);
    void emit_symbol(const PESymbol &symbol);
    void emit_string_table(const PEStringTable &string_table);
};

} // namespace codegen

} // namespace banjo

#endif

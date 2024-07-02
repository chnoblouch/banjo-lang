#include "pe_emitter.hpp"

#include "emit/pe/pe_builder.hpp"
#include "target/x86_64/x86_64_encoder.hpp"
#include "utils/timing.hpp"

namespace banjo {

namespace codegen {

void PEEmitter::generate() {
    PROFILE_SCOPE_BEGIN("x86-64 encoder");
    BinModule bin_module = target::X8664Encoder().encode(module);
    PROFILE_SCOPE_END("x86-64 encoder");

    // TODO: move this elsewhere
    bin_module.drectve_data = WriteBuffer{};

    for (const std::string &dll_export : module.get_dll_exports()) {
        bin_module.drectve_data->write_cstr("/EXPORT:");
        bin_module.drectve_data->write_data((void *)dll_export.data(), dll_export.size());
    }

    PROFILE_SCOPE_BEGIN("PE builder");
    PEFile file = PEBuilder().build(std::move(bin_module));
    PROFILE_SCOPE_END("PE builder");

    PROFILE_SCOPE_BEGIN("PE emitter");
    emit_file(file);
    PROFILE_SCOPE_END("PE emitter");
}

void PEEmitter::emit_file(const PEFile &file) {
    emit_header(file);
    emit_sections(file.sections);
    emit_symbol_table(file.symbols);
    emit_string_table(file.string_table);
}

void PEEmitter::emit_header(const PEFile &file) {
    emit_u16(0x8664);               // machine (x86_64)
    emit_i16(file.sections.size()); // number of sections
    emit_i32(std::time(nullptr));   // low 32 bits of creation timestamp
    emit_i32(0);                    // pointer to symbol table
    emit_i32(file.symbols.size());  // number of symbols
    emit_i16(0);                    // size of optional header
    emit_i16(0);                    // characteristics
}

void PEEmitter::emit_sections(const std::vector<PESection> &sections) {
    std::vector<SectionPointerPositions> pointer_positions;
    pointer_positions.resize(sections.size());

    for (unsigned i = 0; i < sections.size(); i++) {
        emit_section_header(sections[i], pointer_positions[i]);
    }

    for (unsigned i = 0; i < sections.size(); i++) {
        emit_section_data(sections[i].data, pointer_positions[i].data);
    }

    for (unsigned i = 0; i < sections.size(); i++) {
        emit_section_relocations(sections[i].relocations, pointer_positions[i].relocations);
    }
}

void PEEmitter::emit_section_header(const PESection &section, SectionPointerPositions &pointer_positions) {
    stream.write((char *)section.name, 8);
    emit_i32(0);                   // virtual size
    emit_i32(0);                   // virtual address
    emit_i32(section.data.size()); // size of raw data
    pointer_positions.data = tell();
    emit_i32(0); // pointer to raw data
    pointer_positions.relocations = tell();
    emit_i32(0);                          // pointer to relocations
    emit_i32(0);                          // pointer to line numbers
    emit_i16(section.relocations.size()); // number of relocations
    emit_i16(0);                          // number of line numbers
    emit_u32(section.flags);              // characteristics
}

void PEEmitter::emit_section_data(const std::vector<std::uint8_t> &data, std::size_t pointer_pos) {
    // Patch the pointer to the data in the section header.
    std::size_t pos = tell();
    seek(pointer_pos);
    emit_u32(pos);
    seek(pos);

    // Write the data.
    stream.write((char *)data.data(), data.size());
}

void PEEmitter::emit_section_relocations(const std::vector<PERelocation> &relocations, std::size_t pointer_pos) {
    // Patch the pointer to the relocations in the section header.
    std::size_t pos = tell();
    seek(pointer_pos);
    emit_u32(pos);
    seek(pos);

    // Write the relocations.
    for (const PERelocation &relocation : relocations) {
        emit_u32(relocation.virt_addr);
        emit_u32(relocation.symbol_index);
        emit_u16((std::uint16_t)relocation.type);
    }
}

void PEEmitter::emit_symbol_table(const std::vector<PESymbol> &symbols) {
    std::size_t pos = tell();
    seek(8);
    emit_u32(pos);
    seek(pos);

    /*
    TODO: Section symbols usually have auxiliary symbols, but link.exe and lld-link.exe have
    accepted these "broken" object files so far, so do we actually care?
    */

    for (const PESymbol &symbol : symbols) {
        emit_symbol(symbol);
    }
}

void PEEmitter::emit_symbol(const PESymbol &symbol) {
    emit_u64(symbol.name_data);                  // name
    emit_u32(symbol.value);                      // value
    emit_i16(symbol.section_number);             // section number (one-based)
    emit_i16(0);                                 // type
    emit_u8((std::uint8_t)symbol.storage_class); // storage class
    emit_u8(0);                                  // number of auxiliary symbols
}

void PEEmitter::emit_string_table(const PEStringTable &string_table) {
    emit_u32(string_table.size);

    for (const std::string &string : string_table.strings) {
        stream.write(string.data(), string.size());
        char null = '\0';
        stream.write(&null, 1);
    }
}

} // namespace codegen

} // namespace banjo

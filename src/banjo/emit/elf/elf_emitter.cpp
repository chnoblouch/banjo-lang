#include "elf_emitter.hpp"

#include "banjo/emit/elf/elf_builder.hpp"
#include "banjo/target/x86_64/x86_64_encoder.hpp"
#include "banjo/utils/timing.hpp"

namespace banjo {

namespace codegen {

void ELFEmitter::generate() {
    PROFILE_SCOPE_BEGIN("x86-64 encoder");
    BinModule module_ = target::X8664Encoder().encode(module);
    PROFILE_SCOPE_END("x86-64 encoder");

    PROFILE_SCOPE_BEGIN("ELF builder");
    ELFFile file = ELFBuilder().build(std::move(module_));
    PROFILE_SCOPE_END("ELF builder");

    PROFILE_SCOPE_BEGIN("ELF emitter");
    emit_file(file);
    PROFILE_SCOPE_END("ELF emitter");
}

void ELFEmitter::emit_file(const ELFFile &file) {
    emit_header(file);
    emit_sections(file.sections);
}

void ELFEmitter::emit_header(const ELFFile &file) {
    emit_u8('\x7F'); // magic 0
    emit_u8('E');    // magic 1
    emit_u8('L');    // magic 2
    emit_u8('F');    // magic 3
    emit_u8(2);      // class (64-bit object)
    emit_u8(1);      // data encoding (little-endian)
    emit_u8(1);      // version (current)
    emit_u8(0);      // OS + ABI (System V ABI)
    emit_u8(0);      // ABI version

    // Emit padding bytes to get to 16 bytes for the identification.
    for (int i = 0; i < 7; i++) {
        emit_u8(0);
    }

    emit_u16(1);                        // type (relocatable object file)
    emit_u16(file.machine);             // machine (x86_64)
    emit_u32(1);                        // version (current)
    emit_u64(0);                        // entry point address
    emit_u64(0);                        // program header offset
    emit_u64(64);                       // section header offset
    emit_u32(0);                        // flags
    emit_u16(64);                       // header size
    emit_u16(0);                        // size of program header entry
    emit_u16(0);                        // number of program header entries
    emit_u16(64);                       // size of section header entry
    emit_u16(file.sections.size() + 1); // number of section header entries
    emit_u16(3);                        // section name string table index
}

void ELFEmitter::emit_sections(const std::vector<ELFSection> &sections) {
    // Emit first empty entry in section table, which must contain all zeros.
    for (int i = 0; i < 64; i++) {
        emit_u8(0);
    }

    std::vector<SectionPointerPositions> pointer_positions;
    pointer_positions.resize(sections.size());

    for (unsigned i = 0; i < sections.size(); i++) {
        emit_section_header(sections[i], pointer_positions[i]);
    }

    for (unsigned i = 0; i < sections.size(); i++) {
        emit_section_data(sections[i].data, pointer_positions[i].data);
    }
}

void ELFEmitter::emit_section_header(const ELFSection &section, SectionPointerPositions &pointer_positions) {
    emit_u32(section.name_offset); // name offset
    emit_u32(section.type);        // type
    emit_u64(section.flags);       // attributes
    emit_u64(0);                   // virtual address in memory
    pointer_positions.data = tell();
    emit_u64(0); // offset of raw data

    // size of raw data
    if (std::holds_alternative<ELFSection::BinaryData>(section.data)) {
        emit_u64(std::get<ELFSection::BinaryData>(section.data).size());
    } else if (std::holds_alternative<ELFSection::SymbolList>(section.data)) {
        emit_u64(section.entry_size * std::get<ELFSection::SymbolList>(section.data).size());
    } else if (std::holds_alternative<ELFSection::RelocationList>(section.data)) {
        emit_u64(section.entry_size * std::get<ELFSection::RelocationList>(section.data).size());
    }

    emit_u32(section.link);       // link to other section
    emit_u32(section.info);       // miscellaneous information
    emit_u64(section.alignment);  // address alignment
    emit_u64(section.entry_size); // size of entries
}

void ELFEmitter::emit_section_data(const ELFSection::Data &data, std::size_t pointer_pos) {
    // Patch the pointer to the data in the section header.
    std::size_t pos = tell();
    seek(pointer_pos);
    emit_u64(pos);
    seek(pos);

    // Write the data.
    if (std::holds_alternative<ELFSection::BinaryData>(data)) {
        const ELFSection::BinaryData &binary_data = std::get<ELFSection::BinaryData>(data);
        stream.write((char *)binary_data.data(), binary_data.size());
    } else if (std::holds_alternative<ELFSection::SymbolList>(data)) {
        for (const ELFSymbol &symbol : std::get<ELFSection::SymbolList>(data)) {
            emit_u32(symbol.name_offset);                 // name offset
            emit_u8(symbol.type | (symbol.binding << 4)); // type and binding
            emit_u8(0);                                   // reserved byte
            emit_u16(symbol.section_index);               // section index
            emit_u64(symbol.value);                       // value
            emit_u64(symbol.size);                        // size of object
        }
    } else if (std::holds_alternative<ELFSection::RelocationList>(data)) {
        for (const ELFRelocation &relocation : std::get<ELFSection::RelocationList>(data)) {
            emit_u64(relocation.offset); // address of reference
            emit_u64(
                (std::uint64_t)relocation.type | ((std::uint64_t)relocation.symbol_index << 32)
            ); // type and symbol index
            emit_i64(relocation.addend);
        }
    }
}

} // namespace codegen

} // namespace banjo

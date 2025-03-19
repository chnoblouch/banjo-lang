#include "macho_emitter.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/emit/macho/macho_builder.hpp"
#include "banjo/emit/macho/macho_format.hpp"
#include "banjo/target/aarch64/aarch64_encoder.hpp"
#include "banjo/utils/bit_operations.hpp"
#include "banjo/utils/macros.hpp"

#include <cstdint>
#include <utility>
#include <variant>

namespace banjo {
namespace codegen {

void MachOEmitter::generate() {
    markers.clear();
    marker_index = 0;

    BinModule bin_mod = target::AArch64Encoder().encode(module);
    MachOFile file = MachOBuilder().build(std::move(bin_mod));
    emit_file(file);
}

void MachOEmitter::emit_file(const MachOFile &file) {
    emit_headers(file);
    emit_data(file);
}

void MachOEmitter::emit_headers(const MachOFile &file) {
    emit_file_header(file);
    std::size_t command_start = tell();

    for (const MachOCommand &command : file.load_commands) {
        if (const auto *segment = std::get_if<MachOSegment>(&command)) {
            emit_segment_header(*segment);
        } else if (const auto *symtab_command = std::get_if<MachOSymtabCommand>(&command)) {
            emit_symtab_header(*symtab_command);
        } else if (const auto *dysymtab_command = std::get_if<MachODysymtabCommand>(&command)) {
            emit_dysymtab_header(*dysymtab_command);
        } else {
            ASSERT_UNREACHABLE;
        }
    }

    patch_u64(consume_marker(), tell() - command_start);
}

void MachOEmitter::emit_file_header(const MachOFile &file) {
    emit_u32(0xFEEDFACF);                // magic number
    emit_u32(file.cpu_type);             // cpu specifier
    emit_u32(file.cpu_sub_type);         // machine specifier
    emit_u32(0x1);                       // file type (MH_OBJECT)
    emit_u32(file.load_commands.size()); // number of load commands
    emit_placeholder_u32();              // size of all load commands (placeholder)
    emit_u32(0x0);                       // flags
    emit_u32(0x0);                       // reserved
}

void MachOEmitter::emit_segment_header(const MachOSegment &segment) {
    std::size_t start_pos = tell();
    emit_u32(0x19); // command (LC_SEGMENT_64)

    std::size_t cmd_size_pos = tell();
    emit_u32(0); // command size (placeholder)

    emit_name_padded(segment.name); // name
    emit_u64(0);                    // address in memory

    std::uint64_t vm_size_pos = tell();
    emit_u64(0); // size in memory (placeholder)

    emit_placeholder_u64(); // offset in file (placeholder)

    std::uint64_t size_pos = tell();
    emit_u64(0); // size in file (placeholder)

    emit_u32(segment.maximum_vm_prot); // maximum VM protection
    emit_u32(segment.initial_vm_prot); // initial VM protection
    emit_u32(segment.sections.size()); // number of sections
    emit_u32(0);                       // flags

    std::uint64_t total_size = 0;

    for (const MachOSection &section : segment.sections) {
        emit_name_padded(section.name);         // section name
        emit_name_padded(section.segment_name); // segment name

        // TODO: I'm not sure how this address is usually calculated, but does
        // it even matter since the linker throws it away anyway?
        emit_u64(total_size); // address in memory

        emit_u64(section.data.size());        // size in memory
        emit_placeholder_u32();               // offset in file (placeholder)
        emit_u32(0);                          // alignment
        emit_placeholder_u32();               // offset in file of relocation entries (placeholder)
        emit_u32(section.relocations.size()); // number of relocation entries
        emit_u32(section.flags);              // flags
        emit_u32(0);                          // reserved 1
        emit_u32(0);                          // reserved 2
        emit_u32(0);                          // reserved 3

        total_size += section.data.size();
    }

    patch_u32(cmd_size_pos, tell() - start_pos);
    patch_u64(vm_size_pos, total_size);
    patch_u64(size_pos, total_size);
}

void MachOEmitter::emit_symtab_header(const MachOSymtabCommand &command) {
    emit_u32(0x2);                    // command (LC_SYMTAB)
    emit_u32(24);                     // command size
    emit_placeholder_u32();           // symbol table offset (placeholder)
    emit_u32(command.symbols.size()); // number of symbol table entries
    emit_placeholder_u32();           // string table offset (placeholder)
    emit_placeholder_u32();           // string table size (placeholder)
}

void MachOEmitter::emit_dysymtab_header(const MachODysymtabCommand &command) {
    emit_u32(0xB);                             // command (LC_DYSYMTAB)
    emit_u32(80);                              // command size
    emit_u32(command.local_symbols.index);     // index of first local symbol
    emit_u32(command.local_symbols.count);     // number of local symbols
    emit_u32(command.external_symbols.index);  // index of first external symbol
    emit_u32(command.external_symbols.count);  // number of external symbols
    emit_u32(command.undefined_symbols.index); // index of first undefined symbol
    emit_u32(command.undefined_symbols.count); // number of undefined symbols
    emit_u32(0);                               // file offset of table of contents
    emit_u32(0);                               // number of entries in table of contents
    emit_u32(0);                               // file offset of module table
    emit_u32(0);                               // number of entries in module table
    emit_u32(0);                               // file offset of reference symbol table
    emit_u32(0);                               // number of entries in reference symbol table
    emit_u32(0);                               // file offset of indirect symbol table
    emit_u32(0);                               // number of entries in indirect symbol table
    emit_u32(0);                               // file offset of external relocation entries
    emit_u32(0);                               // number of external relocation entries
    emit_u32(0);                               // file offset of local relocation entries
    emit_u32(0);                               // number of local relocation entries
}

void MachOEmitter::emit_data(const MachOFile &file) {
    for (const MachOCommand &command : file.load_commands) {
        if (const MachOSegment *segment = std::get_if<MachOSegment>(&command)) {
            emit_segment_data(*segment);
        } else if (const auto *symtab_command = std::get_if<MachOSymtabCommand>(&command)) {
            emit_symtab_data(*symtab_command);
        }
    }
}

void MachOEmitter::emit_segment_data(const MachOSegment &segment) {
    patch_u64(consume_marker(), tell());

    for (const MachOSection &section : segment.sections) {
        patch_u32(consume_marker(), tell());
        stream.write((char *)section.data.data(), section.data.size());

        patch_u32(consume_marker(), tell());

        for (const MachORelocation &relocation : section.relocations) {
            emit_i32(relocation.address);

            std::uint32_t value_bits = relocation.value;
            std::uint32_t pc_rel_bits = relocation.pc_rel << 24;
            std::uint32_t length_bits = (BitOperations::get_first_bit_set(relocation.length)) << 25;
            std::uint32_t external_bits = relocation.external << 27;
            std::uint32_t type_bits = relocation.type << 28;
            emit_u32(value_bits | pc_rel_bits | length_bits | external_bits | type_bits);
        }
    }
}

void MachOEmitter::emit_symtab_data(const MachOSymtabCommand &command) {
    patch_u32(consume_marker(), tell());

    std::size_t string_table_index = 1;

    for (const MachOSymbol &symbol : command.symbols) {
        emit_u32(string_table_index);    // index of the name in the string table
        emit_u8(0x0E | symbol.external); // type
        emit_u8(symbol.section_number);  // section number
        emit_u16(0);                     // stab description
        emit_u64(symbol.value);          // value

        string_table_index += symbol.name.size() + 1;
    }

    patch_u32(consume_marker(), tell());

    std::size_t string_table_size = 1;

    // If the index in the string table of a symbol name is zero, the symbol is
    // defined to have a null name. This means that we have to insert a byte at
    // the start of the string table so the string table index of the first
    // symbol is 1.
    emit_u8(0);

    for (const MachOSymbol &symbol : command.symbols) {
        stream.write(symbol.name.data(), symbol.name.size());
        emit_u8(0);
        string_table_size += symbol.name.size() + 1;
    }

    // Looking at the output of clang, it seems like the string table size has
    // to be a multiple of 8 bytes.
    while (string_table_size % 8 != 0) {
        emit_u8(0);
        string_table_size += 1;
    }

    patch_u32(consume_marker(), string_table_size);
}

void MachOEmitter::emit_placeholder_u32() {
    push_marker();
    emit_u32(0);
}

void MachOEmitter::emit_placeholder_u64() {
    push_marker();
    emit_u64(0);
}

void MachOEmitter::patch_u32(std::size_t position, std::uint32_t value) {
    std::size_t current_position = tell();
    seek(position);
    emit_u32(value);
    seek(current_position);
}

void MachOEmitter::patch_u64(std::size_t position, std::uint64_t value) {
    std::size_t current_position = tell();
    seek(position);
    emit_u64(value);
    seek(current_position);
}

void MachOEmitter::emit_name_padded(const std::string &name) {
    ASSERT(name.size() <= 16);

    for (char c : name) {
        emit_u8(c);
    }

    for (unsigned i = 0; i < 16 - name.size(); i++) {
        emit_u8(0);
    }
}

void MachOEmitter::push_marker() {
    markers.push_back(tell());
}

std::size_t MachOEmitter::consume_marker() {
    return markers[marker_index++];
}

} // namespace codegen
} // namespace banjo

#include "pe_format.hpp"

#include <cstring>

void PEFile::add_symbol(PESymbolBlueprint blueprint) {
    if (blueprint.name.size() <= 8) {
        PESymbol symbol = {
            .value = blueprint.value,
            .section_number = blueprint.section_number,
            .storage_class = blueprint.storage_class
        };

        char name[8];
        unsigned i = 0;

        // Copy name characters.
        while (i < blueprint.name.size()) {
            name[i] = blueprint.name[i];
            i++;
        }

        // Pad with null bytes.
        while (i < 8) {
            name[i] = '\0';
            i++;
        }

        std::memcpy(&symbol.name_data, name, 8);
        symbols.push_back(symbol);
    } else {
        symbols.push_back(PESymbol{
            // Shift the offset into the string table 32 bits to the left to fill the lower 32 bits with zeros.
            // When emitted in little-endian format, the zeros will be moved to the high 32 bits.
            // These zeros indicate to the linker that this is an offset into the string table.
            .name_data = (std::uint64_t)string_table.size << 32,

            .value = blueprint.value,
            .section_number = blueprint.section_number,
            .storage_class = blueprint.storage_class
        });

        string_table.strings.push_back(blueprint.name);
        string_table.size += blueprint.name.size() + 1;
    }
}

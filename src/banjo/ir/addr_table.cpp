#include "addr_table.hpp"

namespace banjo {

void AddrTable::load(std::istream &stream) {
    std::string name;
    for (unsigned i = 0; std::getline(stream, name); i++) {
        items.insert({name, i});
    }
}

} // namespace banjo

#include "addr_table.hpp"

namespace banjo {

namespace ir {

void AddrTable::load(std::istream &stream) {
    std::string name;
    for (unsigned i = 0; std::getline(stream, name); i++) {
        entries.insert({name, i});
    }
}

} // namespace ir

} // namespace banjo

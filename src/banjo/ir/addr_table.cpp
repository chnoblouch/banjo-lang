#include "addr_table.hpp"

namespace banjo {

namespace ir {

void AddrTable::append(const std::string &symbol) {
    if (indices.contains(symbol)) {
        return;
    }

    indices.insert({symbol, entries.size()});
    entries.push_back(symbol);
}

std::optional<unsigned> AddrTable::find_index(const std::string &symbol) const {
    auto iter = indices.find(symbol);
    return iter == indices.end() ? std::optional<unsigned>{} : iter->second;
}

unsigned AddrTable::compute_offset(unsigned index) {
    unsigned header_size = 4;

    for (const std::string &entry : entries) {
        header_size += 4 + entry.size();
    }

    return header_size + 8 * index;
}

} // namespace ir

} // namespace banjo

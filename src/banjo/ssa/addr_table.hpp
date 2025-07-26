#ifndef BANJO_SSA_ADDR_TABLE_H
#define BANJO_SSA_ADDR_TABLE_H

#include "banjo/ssa/global.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace ssa {

class AddrTable {

public:
    static ssa::GlobalDecl DUMMY_GLOBAL;

private:
    std::vector<std::string> entries;
    std::unordered_map<std::string, unsigned> indices;

public:
    void append(const std::string &symbol);
    std::optional<unsigned> find_index(const std::string &symbol) const;
    unsigned compute_offset(unsigned index);

    const std::vector<std::string> &get_entries() const { return entries; }
};

} // namespace ssa

} // namespace banjo

#endif

#ifndef ADDR_TABLE_H
#define ADDR_TABLE_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace banjo {

namespace ir {

class AddrTable {

private:
    std::vector<std::string> entries;
    std::unordered_map<std::string, unsigned> indices;

public:
    void append(const std::string &symbol);
    std::optional<unsigned> find_index(const std::string &symbol) const;
    unsigned compute_offset(unsigned index);

    const std::vector<std::string> &get_entries() const { return entries; }
};

} // namespace ir

} // namespace banjo

#endif

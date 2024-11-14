#ifndef ADDR_TABLE_H
#define ADDR_TABLE_H

#include <istream>
#include <string>
#include <unordered_map>
#include <utility>

namespace banjo {

namespace ir {

class AddrTable {

private:
    typedef std::unordered_map<std::string, unsigned> Map;

    Map entries;

public:
    void load(std::istream &stream);
    void insert(std::string name, unsigned index) { entries.insert({std::move(name), index}); }

    unsigned get_size() const { return entries.size(); }
    unsigned get(const std::string &name) { return entries[name]; }
    Map::iterator find(const std::string &name) { return entries.find(name); }

    Map::iterator begin() { return entries.begin(); }
    Map::iterator end() { return entries.end(); }
};

} // namespace ir

} // namespace banjo

#endif

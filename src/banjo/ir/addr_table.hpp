#ifndef ADDR_TABLE_H
#define ADDR_TABLE_H

#include <istream>
#include <string>
#include <unordered_map>

namespace banjo {

class AddrTable {

private:
    typedef std::unordered_map<std::string, unsigned> Map;

    Map items;

public:
    void load(std::istream &stream);
    void insert(const std::string name, unsigned index) { items.insert({name, index}); }

    unsigned get(const std::string &name) { return items[name]; }
    Map::iterator find(const std::string &name) { return items.find(name); }

    Map::iterator begin() { return items.begin(); }
    Map::iterator end() { return items.end(); }
};

} // namespace banjo

#endif

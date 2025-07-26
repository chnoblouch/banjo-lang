#ifndef BANJO_SSA_STRUCTURE_H
#define BANJO_SSA_STRUCTURE_H

#include "banjo/ssa/type.hpp"

#include <string>
#include <utility>
#include <vector>

namespace banjo {

namespace ssa {

struct StructureMember {
    std::string name;
    Type type;
};

struct Structure {

public:
    std::string name;
    std::vector<StructureMember> members;

public:
    Structure(std::string name) : name(std::move(name)) {}

    void add(const StructureMember &member) { this->members.push_back(member); }
};

} // namespace ssa

} // namespace banjo

#endif

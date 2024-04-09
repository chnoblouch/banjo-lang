#ifndef IR_STRUCTURE_H
#define IR_STRUCTURE_H

#include "ir/type.hpp"

#include <string>
#include <utility>
#include <vector>

namespace ir {

struct StructureMember {
    std::string name;
    Type type;
};

class Structure {

private:
    std::string name;
    std::vector<StructureMember> members;

public:
    Structure(std::string name) : name(std::move(name)) {}

    std::string get_name() const { return name; }
    const std::vector<StructureMember> &get_members() const { return members; }

    void add(const StructureMember &member) { this->members.push_back(member); }

    int get_member_index(const std::string &name);
};

} // namespace ir

#endif

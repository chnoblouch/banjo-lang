#ifndef ATTRIBUTE_LIST_H
#define ATTRIBUTE_LIST_H

#include "ast/attribute.hpp"
#include <vector>

namespace banjo {

namespace lang {

class AttributeList {

private:
    std::vector<Attribute> attributes;

public:
    const std::vector<Attribute> &get_attributes() { return attributes; }
    void add(Attribute attribute) { this->attributes.push_back(attribute); }
};

} // namespace lang

} // namespace banjo

#endif

#ifndef TYPE_ALIAS_H
#define TYPE_ALIAS_H

#include "banjo/symbol/symbol.hpp"

namespace banjo {

namespace lang {

class DataType;

class TypeAlias : public Symbol {

private:
    DataType *underlying_type = nullptr;

public:
    TypeAlias() : Symbol(nullptr, "") {}
    TypeAlias(ASTNode *node, std::string name) : Symbol(node, name) {}

    DataType *get_underlying_type() const { return underlying_type; }
    void set_underlying_type(DataType *underlying_type) { this->underlying_type = underlying_type; }
};

} // namespace lang

} // namespace banjo

#endif

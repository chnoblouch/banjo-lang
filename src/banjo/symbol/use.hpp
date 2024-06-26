#ifndef USE_H
#define USE_H

#include "symbol/symbol.hpp"
#include "symbol/symbol_ref.hpp"

namespace lang {

class Use : public Symbol {

private:
    SymbolRef target;

public:
    Use() : Symbol(nullptr, "") {}
    Use(ASTNode *node, std::string name) : Symbol(node, name) {}

    const SymbolRef &get_target() const { return target; }
    void set_target(SymbolRef &target) { this->target = target; }
};

} // namespace lang

#endif

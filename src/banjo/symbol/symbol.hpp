#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <utility>
#include <vector>

namespace lang {

class ASTNode;
class ASTModule;
class Identifier;

enum class SymbolVisbility {
    PRIVATE,
    PUBLIC,
};

struct SymbolUse {
    ASTModule *mod;
    Identifier *node;
};

class Symbol {

protected:
    ASTNode *node;
    std::string name;
    SymbolVisbility visibility = SymbolVisbility::PUBLIC;
    std::vector<SymbolUse> uses;

public:
    Symbol(ASTNode *node, std::string name) : node(node), name(std::move(name)) {}

    ASTNode *get_node() { return node; }
    const std::string &get_name() const { return name; }
    SymbolVisbility get_visibility() const { return visibility; }
    const std::vector<SymbolUse> &get_uses() const { return uses; }

    void set_name(const std::string &name) { this->name = name; }
    void set_visibility(SymbolVisbility visibility) { this->visibility = visibility; }
    void add_use(SymbolUse use) { uses.push_back(use); }
};

} // namespace lang

#endif

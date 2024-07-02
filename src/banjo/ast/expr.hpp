#ifndef EXPR_H
#define EXPR_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/symbol/data_type.hpp"
#include "banjo/symbol/function.hpp"
#include "banjo/symbol/location.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/utils/large_int.hpp"

#include <utility>
#include <vector>

namespace banjo {

namespace lang {

class Expr : public ASTNode {

private:
    DataType *data_type = nullptr;
    std::optional<Location> location;
    std::vector<DataType *> coercion_chain;

public:
    Expr(ASTNodeType type, TextRange range = {0, 0}, std::string value = "") : ASTNode(type, std::move(value), range) {}

    DataType *get_data_type() const { return data_type; }
    const std::optional<Location> &get_location() const { return location; }
    const std::vector<DataType *> &get_coercion_chain() const { return coercion_chain; }
    bool is_coerced() const { return !coercion_chain.empty(); }

    void set_data_type(DataType *data_type) { this->data_type = data_type; }
    void set_location(std::optional<Location> location) { this->location = std::move(location); }
    void set_coercion_chain(std::vector<DataType *> coercion_chain) {
        this->coercion_chain = std::move(coercion_chain);
    }

    ASTNode *create_clone() override {
        Expr *clone = new Expr(get_type());
        clone->data_type = data_type;
        clone->coercion_chain = coercion_chain;
        return clone;
    }
};

class IntLiteral : public Expr {

private:
    LargeInt value;

public:
    IntLiteral(LargeInt value) : Expr(AST_INT_LITERAL), value(value) {}
    const LargeInt &get_value() const { return value; }

    ASTNode *create_clone() override {
        IntLiteral *clone = new IntLiteral(value);
        clone->set_data_type(get_data_type());
        return clone;
    }
};

class Identifier : public Expr {

private:
    std::optional<SymbolRef> symbol;

public:
    Identifier(std::string value, TextRange range) : Expr(AST_IDENTIFIER, range, std::move(value)) {}
    Identifier(Token *token) : Identifier(token->get_value(), token->get_range()) {}
    const std::optional<SymbolRef> &get_symbol() const { return symbol; }
    void set_symbol(SymbolRef symbol) { this->symbol = symbol; }

    ASTNode *create_clone() override {
        Identifier *clone = new Identifier(value, range);
        clone->set_data_type(get_data_type());
        return clone;
    }
};

class OperatorExpr : public Expr {

public:
    Function *operator_func = nullptr;

    OperatorExpr(ASTNodeType type, TextRange range) : Expr(type, range) {}
    Function *get_operator_func() { return operator_func; }
    void set_operator_func(Function *operator_func) { this->operator_func = operator_func; }

    ASTNode *create_clone() override { return new OperatorExpr(type, range); }
};

class BracketExpr : public Expr {

public:
    enum Kind {
        INDEX,
        GENERIC_INSTANTIATION,
    };

private:
    Kind kind;

public:
    BracketExpr() : Expr(AST_ARRAY_ACCESS) {}
    Kind get_kind() const { return kind; }
    void set_kind(Kind kind) { this->kind = kind; }

    ASTNode *create_clone() override { return new BracketExpr(); }
};

class MetaExpr : public Expr {

private:
    Expr *value = nullptr;

public:
    MetaExpr(ASTNodeType type) : Expr(type) {}
    ~MetaExpr() { delete value; }

    Expr *get_value() { return value; }
    void set_value(Expr *value) { this->value = value; }

    ASTNode *create_clone() override {
        MetaExpr *clone = new MetaExpr(type);
        clone->value = value;
        return clone;
    }
};

class ClosureExpr : public Expr {

private:
    Function *func;

public:
    ClosureExpr() : Expr(AST_CLOSURE) {}
    Function *get_func() { return func; }
    void set_func(Function *func) { this->func = func; }

    ASTNode *create_clone() override {
        ClosureExpr *clone = new ClosureExpr();
        clone->func = func;
        return clone;
    }
};

} // namespace lang

} // namespace banjo

#endif

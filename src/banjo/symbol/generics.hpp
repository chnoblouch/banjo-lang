#ifndef GENERICS_H
#define GENERICS_H

#include "banjo/symbol/symbol.hpp"

#include <string>
#include <variant>
#include <vector>

namespace banjo {

namespace lang {

class ASTNode;
class ASTModule;
class Function;
class Structure;
class Expr;
class DataType;

template <typename T>
class GenericEntity;

class GenericArg {

private:
    std::variant<Expr *, DataType *> value;

public:
    GenericArg(Expr *expr) : value(expr) {}
    GenericArg(DataType *type) : value(type) {}

    Expr *get_expr() const { return std::get<Expr *>(value); }
    DataType *get_type() const { return std::get<DataType *>(value); }

    bool is_expr() const { return std::holds_alternative<Expr *>(value); }
    bool is_type() const { return std::holds_alternative<DataType *>(value); }
};

typedef std::vector<DataType *> GenericInstanceArgs;

template <typename T>
struct GenericEntityInstance {
    GenericEntity<T> *generic_entity;
    T *entity = nullptr;
    GenericInstanceArgs args;

    GenericEntityInstance(GenericEntity<T> *generic_entity, GenericInstanceArgs args)
      : generic_entity(generic_entity),
        args(args) {}

    ~GenericEntityInstance() { delete entity->get_node(); }
};

struct GenericParam {
    std::string name;
    bool is_param_sequence;
};

template <typename T>
class GenericEntity : public Symbol {

private:
    ASTModule *module_;
    std::vector<GenericParam> generic_params;
    std::vector<GenericEntityInstance<T> *> instances;

public:
    GenericEntity() : Symbol(nullptr, "") {}
    GenericEntity(ASTNode *node, std::string name, ASTModule *module_) : Symbol(node, name), module_(module_) {}

    ~GenericEntity() {
        for (GenericEntityInstance<T> *instance : instances) {
            delete instance;
        }
    }

    ASTModule *get_module() { return module_; }
    std::vector<GenericParam> &get_generic_params() { return generic_params; }
    const std::vector<GenericEntityInstance<T> *> &get_instances() { return instances; }

    void set_generic_params(std::vector<std::string> generic_params) { /* this->generic_params = generic_params; */ }
    void add_instance(GenericEntityInstance<T> *instance) { instances.push_back(instance); }
};

typedef GenericEntity<Function> GenericFunc;
typedef GenericEntity<Structure> GenericStruct;

typedef GenericEntityInstance<Function> GenericFuncInstance;
typedef GenericEntityInstance<Structure> GenericStructInstance;

namespace GenericsUtils {

bool has_sequence(GenericFunc *func);

} // namespace GenericsUtils

} // namespace lang

} // namespace banjo

#endif

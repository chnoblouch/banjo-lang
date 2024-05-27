#ifndef STRUCT_H
#define STRUCT_H

#include "ir/structure.hpp"
#include "symbol/data_type.hpp"
#include "symbol/function.hpp"
#include "symbol/generics.hpp"
#include "symbol/method_table.hpp"
#include "symbol/symbol.hpp"

#include <string_view>
#include <vector>

namespace lang {

class ASTModule;
class SymbolTable;
class Protocol;

class StructField : public Symbol {

private:
    ASTModule *module_;
    DataType *type;
    bool no_deinit = false;

public:
    StructField();
    StructField(ASTNode *node, std::string name, ASTModule *module_);

    ASTModule *get_module() const { return module_; }
    DataType *get_type() const { return type; }
    bool is_no_deinit() const { return no_deinit; }

    void set_type(DataType *type) { this->type = type; }
    void set_no_deinit(bool no_deinit) { this->no_deinit = no_deinit; }
};

struct ProtoImpl {
    Protocol *proto;
    std::string ir_vtable_name;
};

class Structure : public Symbol {

private:
    ASTModule *module_;
    std::vector<StructField *> fields;
    MethodTable method_table;
    SymbolTable *symbol_table;
    std::vector<ProtoImpl> proto_impls;
    std::vector<std::string> generic_params;
    std::vector<GenericInstanceArgs> generic_instances;

    ir::Structure *ir_struct = nullptr;

public:
    Structure();
    Structure(ASTNode *node, std::string name, ASTModule *module_, SymbolTable *symbol_table);
    Structure(const Structure &other);
    Structure(Structure &&other) noexcept;
    ~Structure();

    Structure &operator=(const Structure &other);
    Structure &operator=(Structure &&other) noexcept;

    ASTModule *get_module() { return module_; }
    std::vector<StructField *> &get_fields() { return fields; }
    MethodTable &get_method_table() { return method_table; }
    SymbolTable *get_symbol_table() { return symbol_table; }
    std::vector<ProtoImpl> &get_proto_impls() { return proto_impls; }
    const std::vector<std::string> &get_generic_params() { return generic_params; }
    bool is_generic() { return !generic_params.empty(); }
    const std::vector<GenericInstanceArgs> &get_generic_instances() { return generic_instances; }
    ir::Structure *get_ir_struct() { return ir_struct; }

    void add_field(StructField *field) { fields.push_back(field); }
    void add_proto_impl(Protocol *proto) { proto_impls.push_back({proto, ""}); }
    void set_generic_params(std::vector<std::string> generic_params) { this->generic_params = generic_params; }
    void add_generic_instance(GenericInstanceArgs generic_instance) { generic_instances.push_back(generic_instance); }
    void set_ir_struct(ir::Structure *ir_struct) { this->ir_struct = ir_struct; }

    StructField *get_field(std::string_view name);
    int get_field_index(StructField *field);
    ProtoImpl *get_matching_proto_impl(Protocol *proto);
};

} // namespace lang

#endif

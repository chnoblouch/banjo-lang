#ifndef UNION_H
#define UNION_H

#include "ir/structure.hpp"
#include "symbol/data_type.hpp"
#include "symbol/method_table.hpp"
#include "symbol/symbol.hpp"

#include <optional>
#include <string>
#include <vector>

namespace banjo {

namespace lang {

class SymbolTable;
class Union;

class UnionCaseField : public Symbol {

private:
    DataType *type;

public:
    UnionCaseField(ASTNode *node, std::string name, DataType *type);
    DataType *get_type() { return type; }
};

class UnionCase : public Symbol {

private:
    Union *union_;
    std::vector<UnionCaseField *> fields;

    ir::Structure *ir_struct;

public:
    UnionCase();
    UnionCase(ASTNode *node, std::string name, Union *union_);
    ~UnionCase();

    Union *get_union() { return union_; }
    std::vector<UnionCaseField *> &get_fields() { return fields; }
    ir::Structure *get_ir_struct() { return ir_struct; }

    void set_ir_struct(ir::Structure *ir_struct) { this->ir_struct = ir_struct; }

    UnionCaseField *get_field(unsigned index) { return fields[index]; }
    unsigned get_field_index(UnionCaseField *field);
    std::optional<unsigned> find_field(const std::string &name);
    void add_field(UnionCaseField *field) { fields.push_back(field); }
};

class Union : public Symbol {

private:
    ASTModule *module_;
    MethodTable method_table;
    SymbolTable *symbol_table;
    std::vector<UnionCase *> cases;

    ir::Structure *ir_struct;

public:
    Union();
    Union(ASTNode *node, std::string name, ASTModule *module_, SymbolTable *symbol_table);

    ASTModule *get_module() { return module_; }
    MethodTable &get_method_table() { return method_table; }
    SymbolTable *get_symbol_table() { return symbol_table; }
    const std::vector<UnionCase *> &get_cases() const { return cases; }
    ir::Structure *get_ir_struct() { return ir_struct; }

    void add_case(UnionCase *case_) { cases.push_back(case_); }
    void set_ir_struct(ir::Structure *ir_struct) { this->ir_struct = ir_struct; }

    UnionCase *get_case(unsigned index) { return cases[index]; }
    unsigned get_case_index(UnionCase *case_);
    UnionCase *get_case(const std::string &name);
};

} // namespace lang

} // namespace banjo

#endif

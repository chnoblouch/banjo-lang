#include "union.hpp"

namespace banjo {

namespace lang {

UnionCaseField::UnionCaseField(ASTNode *node, std::string name, DataType *type) : Symbol(node, name), type(type) {}

UnionCase::UnionCase() : Symbol(nullptr, "") {}

UnionCase::UnionCase(ASTNode *node, std::string name, Union *union_) : Symbol(node, name), union_(union_) {}

UnionCase::~UnionCase() {
    for (UnionCaseField *field : fields) {
        delete field;
    }
}

unsigned UnionCase::get_field_index(UnionCaseField *field) {
    for (unsigned i = 0; i < fields.size(); i++) {
        if (fields[i] == field) {
            return i;
        }
    }

    return 0;
}

std::optional<unsigned> UnionCase::find_field(const std::string &name) {
    for (unsigned i = 0; i < fields.size(); i++) {
        if (fields[i]->get_name() == name) {
            return i;
        }
    }

    return {};
}

Union::Union() : Symbol(nullptr, "") {}

Union::Union(ASTNode *node, std::string name, ASTModule *module_, SymbolTable *symbol_table)
  : Symbol(node, name),
    module_(module_),
    symbol_table(symbol_table) {}

unsigned Union::get_case_index(UnionCase *case_) {
    for (unsigned i = 0; i < cases.size(); i++) {
        if (cases[i] == case_) {
            return i;
        }
    }

    return 0;
}

UnionCase *Union::get_case(const std::string &name) {
    for (UnionCase *case_ : cases) {
        if (case_->get_name() == name) {
            return case_;
        }
    }

    return nullptr;
}

} // namespace lang

} // namespace banjo

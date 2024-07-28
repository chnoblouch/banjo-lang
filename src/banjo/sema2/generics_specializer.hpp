#ifndef GENERICS_SPECIALIZER_H
#define GENERICS_SPECIALIZER_H

#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

class GenericsSpecializer {

private:
    SemanticAnalyzer &analyzer;

public:
    GenericsSpecializer(SemanticAnalyzer &analyzer);
    sir::FuncDef *specialize(sir::FuncDef &generic_func_def, const std::vector<sir::Expr> &args);
    sir::StructDef *specialize(sir::StructDef &generic_struct_def, const std::vector<sir::Expr> &args);

private:
    sir::FuncDef *create_specialized_clone(sir::FuncDef &generic_func_def, const std::vector<sir::Expr> &args);
    sir::StructDef *create_specialized_clone(sir::StructDef &generic_struct_def, const std::vector<sir::Expr> &args);

    template <typename T>
    T *find_existing_specialization(T &generic_def, const std::vector<sir::Expr> &args) {
        for (sir::Specialization<T> &specialization : generic_def.specializations) {
            if (specialization.args == args) {
                return specialization.def;
            }
        }

        return nullptr;
    }
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif

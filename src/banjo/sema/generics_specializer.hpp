#ifndef BANJO_SEMA_GENERICS_SPECIALIZER_H
#define BANJO_SEMA_GENERICS_SPECIALIZER_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/utils.hpp"

namespace banjo {

namespace lang {

namespace sema {

class GenericsSpecializer {

private:
    SemanticAnalyzer &analyzer;

public:
    GenericsSpecializer(SemanticAnalyzer &analyzer);
    sir::FuncDef *specialize(sir::FuncDef &generic_func_def, std::span<sir::Expr> args);
    sir::StructDef *specialize(sir::StructDef &generic_struct_def, std::span<sir::Expr> args);

private:
    sir::FuncDef *create_specialized_clone(sir::FuncDef &generic_func_def, std::span<sir::Expr> args);
    sir::StructDef *create_specialized_clone(sir::StructDef &generic_struct_def, std::span<sir::Expr> args);

    template <typename T>
    T *find_existing_specialization(T &generic_def, std::span<sir::Expr> args) {
        for (sir::Specialization<T> &specialization : generic_def.specializations) {
            if (Utils::equal(specialization.args, args)) {
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

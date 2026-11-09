#include "type_concretizer.hpp"
#include "banjo/sir/expr_substitution.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::sema {

TypeConcretizer::TypeConcretizer(SemanticAnalyzer &analyzer) : DeclVisitor{analyzer} {}

void TypeConcretizer::analyze_decl_block(sir::DeclBlock &decl_block) {
    for (auto [name, symbol] : decl_block.symbol_table->symbols) {
        if (auto proto_method_impl = symbol.match<sir::DefaultMethod>()) {
            concretize_type(*proto_method_impl);
        }
    }
}

void TypeConcretizer::concretize_type(sir::DefaultMethod &impl) {
    utils::Arena &arena = analyzer.mod->trivial_arena;

    sir::Expr self_type = arena.create<sir::SymbolExpr>({
        .ast_node = nullptr,
        .type = nullptr,
        .symbol = analyzer.get_decl(),
    });

    impl.type = sir::concretize_type(arena, impl.type, self_type);
}

} // namespace banjo::sema

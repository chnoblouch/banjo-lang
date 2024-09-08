#include "banjo/sema2/preamble.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/symbol/module_path.hpp"

#include <string>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

struct PreambleDecl {
    ModulePath mod_path;
    std::vector<std::string> names;
};

static const std::vector<PreambleDecl> PREAMBLE_DECLS = {
    {ModulePath{"internal", "preamble"}, {"print", "println", "assert"}},
    {ModulePath{"std", "optional"}, {"Optional"}},
    {ModulePath{"std", "array"}, {"Array"}},
    {ModulePath{"std", "string"}, {"String"}},
    {ModulePath{"std", "set"}, {"Set"}},
    {ModulePath{"std", "closure"}, {"Closure"}},
};

Preamble::Preamble(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void Preamble::insert() {
    for (sir::Module *mod : analyzer.sir_unit.mods) {
        analyzer.enter_mod(mod);
        insert_into(*mod);
        analyzer.exit_mod();
    }
}

void Preamble::insert_into(sir::Module &mod) {
    for (const PreambleDecl &decl : PREAMBLE_DECLS) {
        if (decl.mod_path == mod.path) {
            continue;
        }

        sir::UseItem lhs = create_use_ident(decl.mod_path[0]);

        for (unsigned i = 1; i < decl.mod_path.get_size(); i++) {
            lhs = analyzer.create_use_item(sir::UseDotExpr{
                .ast_node = nullptr,
                .lhs = lhs,
                .rhs = create_use_ident(decl.mod_path[i]),
            });
        }

        std::vector<sir::UseItem> rhs_items;
        rhs_items.resize(decl.names.size());

        for (unsigned i = 0; i < decl.names.size(); i++) {
            rhs_items[i] = create_use_ident(decl.names[i]);
        }

        sir::UseItem rhs = analyzer.create_use_item(sir::UseList{
            .ast_node = nullptr,
            .items = rhs_items,
        });

        sir::UseDecl *use_decl = analyzer.create_decl(sir::UseDecl{
            .ast_node = nullptr,
            .root_item = analyzer.create_use_item(sir::UseDotExpr{
                .ast_node = nullptr,
                .lhs = lhs,
                .rhs = rhs,
            }),
        });

        mod.block.decls.insert(mod.block.decls.begin(), use_decl);
    }
}

sir::UseIdent *Preamble::create_use_ident(const std::string &value) {
    sir::Ident ident{
        .ast_node = nullptr,
        .value = value,
    };

    return analyzer.create_use_item(sir::UseIdent{
        .ident = ident,
        .symbol = nullptr,
    });
}

} // namespace sema

} // namespace lang

} // namespace banjo

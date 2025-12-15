#ifndef BANJO_SEMA_SYMBOL_COLLECTOR_H
#define BANJO_SEMA_SYMBOL_COLLECTOR_H

#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"

#include <optional>
#include <vector>

namespace banjo {

namespace lang {

namespace sema {

class SymbolCollector {

private:
    SemanticAnalyzer &analyzer;
    std::optional<unsigned> guarded_scope;

public:
    SymbolCollector(SemanticAnalyzer &analyzer);
    
    void collect(const std::vector<sir::Module *> &mods);
    void collect_in_mod(sir::Module &mod);
    void collect_in_block(sir::DeclBlock &decl_block);
    void collect_func_specialization(sir::FuncDef &generic_def, sir::FuncDef &specialization);
    void collect_struct_specialization(sir::StructDef &generic_def, sir::StructDef &specialization);
    void collect_closure_def(sir::FuncDef &func_def);

    void collect_decl(sir::Decl &decl, unsigned index);
    void collect_func_def(sir::FuncDef &func_def);
    void collect_func_decl(sir::FuncDecl &func_decl);
    void collect_native_func_decl(sir::NativeFuncDecl &native_func_decl);
    void collect_const_def(sir::ConstDef &const_def);
    void collect_struct_def(sir::StructDef &struct_def);
    void collect_var_decl(sir::VarDecl &var_decl);
    void collect_native_var_decl(sir::NativeVarDecl &native_var_decl);
    void collect_enum_def(sir::EnumDef &enum_def);
    void collect_enum_variant(sir::EnumVariant &enum_variant);
    void collect_union_def(sir::UnionDef &union_def);
    void collect_union_case(sir::UnionCase &union_case);
    void collect_proto_def(sir::ProtoDef &proto_def);
    void collect_type_alias(sir::TypeAlias &type_alias);
    void collect_use_decl(sir::UseDecl &use_decl);
    void collect_in_meta_if_stmt(sir::MetaIfStmt &meta_if_stmt, unsigned index);

    void collect_use_item(sir::UseItem &use_item, sir::UseDecl &parent);
    void collect_use_ident(sir::UseIdent &use_ident, sir::UseDecl &parent);
    void collect_use_rebind(sir::UseRebind &use_rebind, sir::UseDecl &parent);
    void collect_use_dot_expr(sir::UseDotExpr &use_dot_expr, sir::UseDecl &parent);
    void collect_use_list(sir::UseList &use_list, sir::UseDecl &parent);

    void add_symbol(std::string_view name, sir::Symbol symbol);
    sir::SymbolTable &get_symbol_table();
};

} // namespace sema

} // namespace lang

} // namespace banjo

#endif

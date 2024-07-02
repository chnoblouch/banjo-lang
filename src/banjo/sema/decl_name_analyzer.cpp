#include "decl_name_analyzer.hpp"

#include "banjo/ast/ast_child_indices.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/ast_utils.hpp"
#include "banjo/ast/decl.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/sema/use_collector.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/symbol/symbol_table.hpp"
#include "banjo/symbol/union.hpp"

namespace banjo {

namespace lang {

DeclNameAnalyzer::DeclNameAnalyzer(SemanticAnalyzer &sema, bool modify_symbol_table /* = true */)
  : sema(sema),
    modify_symbol_table(modify_symbol_table) {}

void DeclNameAnalyzer::run(ModuleList &module_list) {
    for (ASTModule *module_ : module_list) {
        sema.get_context().push_ast_context().cur_module = module_;

        for (ASTNode *node : module_->get_block()->get_children()) {
            analyze_symbol(node);
        }

        sema.get_context().pop_ast_context();
    }
}

void DeclNameAnalyzer::run(ASTBlock *block) {
    for (ASTNode *node : block->get_children()) {
        analyze_symbol(node);
    }
}

void DeclNameAnalyzer::analyze_symbol(ASTNode *node) {
    if (node->get_sema_stage() != SemaStage::NONE) {
        return;
    }
    node->set_sema_stage(SemaStage::NAME);

    switch (node->get_type()) {
        case AST_FUNCTION_DEFINITION: analyze_func(node->as<ASTFunc>()); break;
        case AST_NATIVE_FUNCTION_DECLARATION: analyze_func(node->as<ASTFunc>()); break;
        case AST_CONSTANT: analyze_const(node->as<ASTConst>()); break;
        case AST_VAR: analyze_var(node->as<ASTVar>()); break;
        case AST_IMPLICIT_TYPE_VAR: analyze_implicit_type_var(node->as<ASTVar>()); break;
        case AST_NATIVE_VAR: analyze_var(node->as<ASTVar>()); break;
        case AST_STRUCT_DEFINITION: analyze_struct(node->as<ASTStruct>()); break;
        case AST_ENUM_DEFINITION: analyze_enum(node->as<ASTEnum>()); break;
        case AST_UNION: analyze_union(node->as<ASTUnion>()); break;
        case AST_PROTO: analyze_proto(node->as<ASTProto>()); break;
        case AST_TYPE_ALIAS: analyze_type_alias(node->as<ASTTypeAlias>()); break;
        case AST_GENERIC_FUNCTION_DEFINITION: analyze_generic_func(node->as<ASTGenericFunc>()); break;
        case AST_GENERIC_STRUCT_DEFINITION: analyze_generic_struct(node->as<ASTGenericStruct>()); break;
        case AST_USE: UseCollector(sema.get_context()).run(node); break;
        default: break;
    }
}

void DeclNameAnalyzer::analyze_func(ASTFunc *node) {
    ASTNode *qualifier_list = node->get_child(FUNC_QUALIFIERS);
    Identifier *name_node = node->get_child(FUNC_NAME)->as<Identifier>();

    const std::string &name = name_node->get_value();
    ASTModule *mod = sema.get_context().get_ast_context().cur_module;
    node->set_symbol({node, name, mod});
    name_node->set_symbol(node->get_symbol());

    for (ASTNode *qualifier_node : qualifier_list->get_children()) {
        if (qualifier_node->get_value() == "pub") {
            node->get_symbol()->set_visibility(SymbolVisbility::PUBLIC);
        }
    }

    Function *func = node->get_symbol();
    func->set_enclosing_symbol(sema.get_context().get_ast_context().enclosing_symbol);

    if (!modify_symbol_table) {
        return;
    }

    if (ASTUtils::is_func_method(node)) {
        if (func->get_enclosing_symbol().get_kind() == SymbolKind::STRUCT) {
            func->get_enclosing_symbol().get_struct()->get_method_table().add_function(func);
        } else if (func->get_enclosing_symbol().get_kind() == SymbolKind::UNION) {
            func->get_enclosing_symbol().get_union()->get_method_table().add_function(func);
        }
    } else {
        sema.get_context().get_ast_context().get_cur_symbol_table()->add_function(func);
    }
}

void DeclNameAnalyzer::analyze_const(ASTConst *node) {
    Identifier *name_node = node->get_child(CONST_NAME)->as<Identifier>();

    const std::string &name = name_node->get_value();
    ASTModule *mod = sema.get_context().get_ast_context().cur_module;
    node->set_symbol({node, name, mod});
    name_node->set_symbol(node->get_symbol());

    sema.get_context().get_ast_context().get_cur_symbol_table()->add_constant(node->get_symbol());
}

void DeclNameAnalyzer::analyze_var(ASTVar *node) {
    Identifier *name_node = node->get_child(VAR_NAME)->as<Identifier>();

    const std::string &name = name_node->get_value();
    ASTModule *mod = sema.get_context().get_ast_context().cur_module;
    SymbolTable *symbol_table = sema.get_context().get_ast_context().get_cur_symbol_table();
    SymbolRef enclosing_symbol = sema.get_context().get_ast_context().enclosing_symbol;

    if (enclosing_symbol.get_kind() == SymbolKind::STRUCT) {
        StructField *field = new StructField(node, name, mod);
        node->set_symbol(field);
        name_node->set_symbol(field);
        symbol_table->add_symbol(field->get_name(), field);
        enclosing_symbol.get_struct()->add_field(field);
    } else {
        GlobalVariable *global = new GlobalVariable(node, name, mod);
        node->set_symbol(global);
        name_node->set_symbol(global);
        symbol_table->add_global_variable(global);
    }
}

void DeclNameAnalyzer::analyze_implicit_type_var(ASTVar *node) {
    if (sema.get_context().get_ast_context().enclosing_symbol.get_kind() == SymbolKind::STRUCT) {
        ASTNode *field_name_node = node->get_child(TYPE_INFERRED_VAR_NAME);
        sema.get_context().register_error(node, ReportText::ID::ERR_FIELD_MISSING_TYPE, field_name_node);
    }
}

void DeclNameAnalyzer::analyze_struct(ASTStruct *node) {
    Identifier *name_node = node->get_child(STRUCT_NAME)->as<Identifier>();
    ASTNode *block_node = node->get_child(STRUCT_BLOCK);

    const std::string &name = name_node->get_value();
    ASTModule *mod = sema.get_context().get_ast_context().cur_module;
    SymbolTable *symbol_table = block_node->as<ASTBlock>()->get_symbol_table();

    node->set_symbol(Structure(node, name, mod, symbol_table));
    name_node->set_symbol(node->get_symbol());

    if (modify_symbol_table) {
        sema.get_context().get_ast_context().get_cur_symbol_table()->add_structure(node->get_symbol());
    }

    sema.get_context().push_ast_context().enclosing_symbol = node->get_symbol();
    for (ASTNode *child : block_node->get_children()) {
        sema.run_name_stage(child);
    }
    sema.get_context().pop_ast_context();
}

void DeclNameAnalyzer::analyze_enum(ASTEnum *node) {
    Identifier *name_node = node->get_child(ENUM_NAME)->as<Identifier>();

    const std::string &name = name_node->get_value();
    ASTModule *mod = sema.get_context().get_ast_context().cur_module;
    SymbolTable *parent_symbol_table = ASTUtils::get_module_symbol_table(mod);

    node->set_symbol(Enumeration(node, name, mod, parent_symbol_table));
    name_node->set_symbol(node->get_symbol());

    sema.get_context().get_ast_context().get_cur_symbol_table()->add_enumeration(node->get_symbol());
}

void DeclNameAnalyzer::analyze_union(ASTUnion *node) {
    Identifier *name_node = node->get_child(UNION_NAME)->as<Identifier>();
    ASTNode *block_node = node->get_child(UNION_BLOCK);

    const std::string &name = name_node->get_value();
    ASTModule *mod = sema.get_context().get_ast_context().cur_module;
    SymbolTable *symbol_table = block_node->as<ASTBlock>()->get_symbol_table();

    node->set_symbol(Union(node, name, mod, symbol_table));
    name_node->set_symbol(node->get_symbol());

    for (ASTNode *child : block_node->get_children()) {
        if (child->get_type() != AST_UNION_CASE) {
            continue;
        }

        ASTUnionCase *case_node = child->as<ASTUnionCase>();
        Identifier *case_name_node = case_node->get_child(UNION_CASE_NAME)->as<Identifier>();

        const std::string &case_name = case_name_node->get_value();
        case_node->set_symbol(UnionCase(child, case_name, node->get_symbol()));
        case_name_node->set_symbol(case_node->get_symbol());

        node->get_symbol()->add_case(case_node->get_symbol());
        symbol_table->add_symbol(case_node->get_symbol()->get_name(), case_node->get_symbol());
    }

    sema.get_context().get_ast_context().get_cur_symbol_table()->add_union(node->get_symbol());

    sema.get_context().push_ast_context().enclosing_symbol = node->get_symbol();
    for (ASTNode *child : block_node->get_children()) {
        sema.run_name_stage(child);
    }
    sema.get_context().pop_ast_context();
}

void DeclNameAnalyzer::analyze_proto(ASTProto *node) {
    Identifier *name_node = node->get_child(PROTO_NAME)->as<Identifier>();

    const std::string &name = name_node->get_value();
    node->set_symbol(Protocol(node, name));
    name_node->set_symbol(node->get_symbol());

    sema.get_context().get_ast_context().get_cur_symbol_table()->add_protocol(node->get_symbol());
}

void DeclNameAnalyzer::analyze_type_alias(ASTTypeAlias *node) {
    Identifier *name_node = node->get_child(TYPE_ALIAS_NAME)->as<Identifier>();

    const std::string &name = name_node->get_value();
    node->set_symbol(TypeAlias(node, name));
    name_node->set_symbol(node->get_symbol());

    sema.get_context().get_ast_context().get_cur_symbol_table()->add_type_alias(node->get_symbol());
}

void DeclNameAnalyzer::analyze_generic_func(ASTGenericFunc *node) {
    Identifier *name_node = node->get_child(GENERIC_FUNC_NAME)->as<Identifier>();

    const std::string &name = name_node->get_value();
    ASTModule *mod = sema.get_context().get_ast_context().cur_module;

    node->set_symbol(GenericFunc(node, name, mod));
    name_node->set_symbol(node->get_symbol());

    sema.get_context().get_ast_context().get_cur_symbol_table()->add_generic_func(node->get_symbol());
}

void DeclNameAnalyzer::analyze_generic_struct(ASTGenericStruct *node) {
    Identifier *name_node = node->get_child(GENERIC_STRUCT_NAME)->as<Identifier>();

    const std::string &name = name_node->get_value();
    ASTModule *mod = sema.get_context().get_ast_context().cur_module;

    node->set_symbol(GenericStruct(node, name, mod));
    name_node->set_symbol(node->get_symbol());

    sema.get_context().get_ast_context().get_cur_symbol_table()->add_generic_struct(node->get_symbol());
}

} // namespace lang

} // namespace banjo

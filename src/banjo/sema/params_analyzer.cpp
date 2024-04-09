#include "params_analyzer.hpp"

#include "ast/ast_child_indices.hpp"
#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "sema/deinit_analyzer.hpp"
#include "sema/type_analyzer.hpp"

namespace lang {

ParamsAnalyzer::ParamsAnalyzer(SemanticAnalyzerContext &context) : context(context) {}

bool ParamsAnalyzer::analyze(ASTNode *node, ASTBlock *block) {
    SymbolTable *symbol_table = block->get_symbol_table();

    for (unsigned i = 0; i < node->get_children().size(); i++) {
        ASTNode *child = node->get_child(i);
        ASTNode *name_node = child->get_child(PARAM_NAME);

        if (check_redefinition(symbol_table, name_node)) {
            return false;
        }

        bool is_self = name_node->get_type() == AST_SELF;
        if (is_self && i != 0) {
            context.register_error(ReportText(ReportText::ID::ERR_SELF_NOT_FIRST_PARAM).str(), name_node);
            return false;
        }

        TypeAnalyzer::Result type_result = TypeAnalyzer(context).analyze_param(child);
        if (type_result.result != SemaResult::OK) {
            return false;
        }
        DataType *type = type_result.type;

        const std::string &name = is_self ? "self" : name_node->get_value();
        Parameter *param = new Parameter(child, type, name);

        if (name_node->get_type() == AST_IDENTIFIER) {
            name_node->as<Identifier>()->set_symbol(param);
        }

        symbol_table->add_parameter(param);

        DeinitAnalyzer(context).analyze_param(block, param);

        if (child->get_attribute_list()) {
            for (const Attribute &attribute : child->get_attribute_list()->get_attributes()) {
                if (attribute.get_name() == "unmanaged") {
                    param->get_deinit_info().set_unmanaged();
                }
            }
        }
    }

    return true;
}

bool ParamsAnalyzer::check_redefinition(SymbolTable *symbol_table, ASTNode *name_node) {
    const std::string name = name_node->get_type() == AST_SELF ? "self" : name_node->get_value();

    std::optional<SymbolRef> existing_symbol = symbol_table->get_symbol(name);
    if (!existing_symbol) {
        return false;
    }

    SymbolKind kind = existing_symbol->get_kind();
    if (kind == SymbolKind::PARAM) {
        std::string message = ReportText(ReportText::ID::ERR_REDEFINITION).format(name).str();
        context.register_error(message, name_node);
        return true;
    }

    return false;
}

} // namespace lang

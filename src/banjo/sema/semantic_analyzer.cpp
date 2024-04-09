#include "semantic_analyzer.hpp"

#include "ast/ast_module.hpp"
#include "ast/ast_utils.hpp"
#include "sema/meta_lowerer.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "sema/stages/decl_body_analyzer.hpp"
#include "sema/stages/decl_name_analyzer.hpp"
#include "sema/stages/decl_type_analyzer.hpp"
#include "sema/stages/use_collector.hpp"
#include "symbol/standard_types.hpp"
#include "symbol/use.hpp"
#include "utils/timing.hpp"

namespace lang {

SemanticAnalyzer::SemanticAnalyzer(ModuleManager &module_manager, DataTypeManager &type_manager, target::Target *target)
  : context(*this, module_manager, type_manager, target) {}

void SemanticAnalyzer::run_name_stage(ASTBlock *block) {
    DeclNameAnalyzer(*this).run(block);
}

void SemanticAnalyzer::run_meta_counting_stage(ASTBlock *block) {
    for (ASTNode *node : block->get_children()) {
        if (node->get_type() == AST_META_IF) {
            context.add_meta_stmt(block->get_symbol_table());
        }
    }
}

void SemanticAnalyzer::run_use_resolution_stage(ASTBlock *block) {
    for (ASTNode *node : block->get_children()) {
        run_use_resolution_stage(node);
    }
}

void SemanticAnalyzer::run_meta_expansion_stage(ASTBlock *block) {
    PROFILE_SCOPE("meta expansion");

    for (unsigned i = 0; i < block->get_children().size(); i++) {
        ASTNode *node = block->get_child(i);

        if (node->get_type() == AST_META_IF) {
            MetaLowerer(context).lower_meta_if(node, 0);
        }
    }
}

void SemanticAnalyzer::run_type_stage(ASTNode *node) {
    Symbol *symbol = ASTUtils::get_symbol(node);

    if (symbol && node->get_sema_stage() == SemaStage::NAME) {
        DeclTypeAnalyzer(*this).run(symbol);
    }
}

void SemanticAnalyzer::run_body_stage(ASTNode *node) {
    Symbol *symbol = ASTUtils::get_symbol(node);

    if (symbol && node->get_sema_stage() == SemaStage::TYPE) {
        DeclBodyAnalyzer(*this).run(symbol);
    }
}

void SemanticAnalyzer::run_name_stage(ASTModule *module_) {
    context.push_ast_context().cur_module = module_;
    run_name_stage(module_->get_block()->as<ASTBlock>());
    context.pop_ast_context();
}

void SemanticAnalyzer::run_name_stage(ASTNode *node, bool modify_symbol_table /* = true */) {
    DeclNameAnalyzer(*this, modify_symbol_table).analyze_symbol(node);
}

void SemanticAnalyzer::run_use_resolution_stage(ASTNode *node) {
    PROFILE_SCOPE("use resolution");

    if (node->get_type() == AST_USE) {
        UseCollector(context).resolve_names(node);
    }
}

std::optional<SymbolRef> SemanticAnalyzer::resolve_symbol(SymbolTable *symbol_table, const std::string &name) {
    std::optional<SymbolRef> symbol = symbol_table->get_symbol_unresolved(name);
    if (!symbol) {
        return {};
    }

    if (symbol->get_kind() == SymbolKind::USE) {
        context.get_sema().run_use_resolution_stage(symbol->get_use()->get_node());
        symbol = symbol->resolve();

        if (!symbol || symbol->get_kind() == SymbolKind::NONE) {
            return {};
        }
    }

    return symbol;
}

SemanticAnalysis SemanticAnalyzer::analyze() {
    PROFILE_SCOPE_BEGIN("name analysis");

    for_each_module([this](ASTModule *module_) {
        for (ASTNode *child : module_->get_block()->get_children()) {
            run_name_stage(child);
        }
    });

    PROFILE_SCOPE_END("name analysis");

    for_each_module([this](ASTModule *module_) { run_meta_expansion_stage(module_->get_block()); });
    for_each_module([this](ASTModule *module_) { run_use_resolution_stage(module_->get_block()); });

    while (!context.get_new_modules().empty()) {
        std::vector<ASTModule *> new_modules_copy = context.get_new_modules();
        context.get_new_modules().clear();

        for_each_module(new_modules_copy, [this](ASTModule *module_) {
            run_meta_expansion_stage(module_->get_block());
        });

        for_each_module(new_modules_copy, [this](ASTModule *module_) {
            run_use_resolution_stage(module_->get_block());
        });
    }

    for_each_module([this](ASTModule *module_) {
        SymbolTable *symbol_table = module_->get_block()->get_symbol_table();
        StandardTypes::link_symbols(symbol_table, context.get_module_manager().get_module_list());
    });

    PROFILE_SCOPE_BEGIN("type analysis");

    for_each_module([this](ASTModule *module_) {
        for (ASTNode *child : module_->get_block()->get_children()) {
            run_type_stage(child);
        }
    });

    PROFILE_SCOPE_END("type analysis");
    PROFILE_SCOPE_BEGIN("body analysis");

    for_each_module([this](ASTModule *module_) {
        for (ASTNode *child : module_->get_block()->get_children()) {
            run_body_stage(child);
        }
    });

    while (!context.get_new_nodes().empty()) {
        std::vector<std::pair<ASTNode *, ASTContext>> new_nodes = context.get_new_nodes();
        context.get_new_nodes().clear();

        for (auto &[node, ast_context] : new_nodes) {
            context.push_ast_context() = ast_context;
            run_body_stage(node);
            context.pop_ast_context();
        }
    }

    PROFILE_SCOPE_END("body analysis");

    context.record_new_reports();
    return context.get_analysis();
}

SemanticAnalysis SemanticAnalyzer::analyze_modules(const std::vector<ASTModule *> &module_nodes) {
    for_each_module(module_nodes, [this](ASTModule *module_) {
        for (ASTNode *child : module_->get_block()->get_children()) {
            run_name_stage(child);
        }
    });

    for_each_module(module_nodes, [this](ASTModule *module_) { run_meta_expansion_stage(module_->get_block()); });
    for_each_module(module_nodes, [this](ASTModule *module_) { run_use_resolution_stage(module_->get_block()); });

    while (!context.get_new_modules().empty()) {
        std::vector<ASTModule *> new_modules_copy = context.get_new_modules();
        context.get_new_modules().clear();

        for_each_module(new_modules_copy, [this](ASTModule *module_) {
            run_meta_expansion_stage(module_->get_block());
        });

        for_each_module(new_modules_copy, [this](ASTModule *module_) {
            run_use_resolution_stage(module_->get_block());
        });
    }

    for_each_module(module_nodes, [this](ASTModule *module_) {
        SymbolTable *symbol_table = module_->get_block()->get_symbol_table();
        StandardTypes::link_symbols(symbol_table, context.get_module_manager().get_module_list());
    });

    for_each_module(module_nodes, [this](ASTModule *module_) {
        for (ASTNode *child : module_->get_block()->get_children()) {
            run_type_stage(child);
        }
    });

    for_each_module(module_nodes, [this](ASTModule *module_) {
        for (ASTNode *child : module_->get_block()->get_children()) {
            run_body_stage(child);
        }
    });

    while (!context.get_new_nodes().empty()) {
        std::vector<std::pair<ASTNode *, ASTContext>> new_nodes = context.get_new_nodes();
        context.get_new_nodes().clear();

        for (auto &[node, ast_context] : new_nodes) {
            context.push_ast_context() = ast_context;
            run_body_stage(node);
            context.pop_ast_context();
        }
    }

    context.record_new_reports();
    return context.get_analysis();
}

SemanticAnalysis SemanticAnalyzer::analyze_module(ASTModule *module_) {
    context.push_ast_context().cur_module = static_cast<lang::ASTModule *>(module_);

    for (ASTNode *child : module_->get_block()->get_children()) {
        run_name_stage(child);
    }

    run_meta_expansion_stage(module_->get_block());
    run_use_resolution_stage(module_->get_block());

    while (!context.get_new_modules().empty()) {
        std::vector<ASTModule *> new_modules_copy = context.get_new_modules();
        context.get_new_modules().clear();

        for_each_module(new_modules_copy, [this](ASTModule *module_) {
            run_meta_expansion_stage(module_->get_block());
        });

        for_each_module(new_modules_copy, [this](ASTModule *module_) {
            run_use_resolution_stage(module_->get_block());
        });
    }

    SymbolTable *symbol_table = module_->get_block()->get_symbol_table();
    StandardTypes::link_symbols(symbol_table, context.get_module_manager().get_module_list());

    for (ASTNode *child : module_->get_block()->get_children()) {
        run_type_stage(child);
    }

    for (ASTNode *child : module_->get_block()->get_children()) {
        run_body_stage(child);
    }

    while (!context.get_new_nodes().empty()) {
        std::vector<std::pair<ASTNode *, ASTContext>> new_nodes = context.get_new_nodes();
        context.get_new_nodes().clear();

        for (auto &[node, ast_context] : new_nodes) {
            context.push_ast_context() = ast_context;
            run_body_stage(node);
            context.pop_ast_context();
        }
    }

    context.record_new_reports();

    context.pop_ast_context();
    return context.get_analysis();
}

void SemanticAnalyzer::enable_dependency_tracking() {
    context.set_track_dependencies(true);
}

void SemanticAnalyzer::enable_symbol_use_tracking() {
    context.set_track_symbol_uses(true);
}

void SemanticAnalyzer::for_each_module(const std::function<void(ASTModule *module_)> &function) {
    ModuleList &module_list = context.get_module_manager().get_module_list();

    for (auto iter = module_list.begin(); iter != module_list.end(); ++iter) {
        context.push_ast_context().cur_module = *iter;
        function(*iter);
        context.pop_ast_context();
    }
}

void SemanticAnalyzer::for_each_module(
    const std::vector<ASTModule *> &modules,
    const std::function<void(ASTModule *module_)> &function
) {
    for (ASTModule *module_ : modules) {
        context.push_ast_context().cur_module = module_;
        function(module_);
        context.pop_ast_context();
    }
}

} // namespace lang

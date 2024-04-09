#include "source_manager.hpp"

#include "ast/ast_utils.hpp"
#include "config/config.hpp"
#include "lexer/lexer.hpp"
#include "sema/semantic_analyzer.hpp"
#include "source/module_discovery.hpp"

#include <fstream>
#include <sstream>
#include <utility>

namespace lsp {

SourceManager::SourceManager() : module_loader(*this), module_manager(module_loader, report_manager) {}

void SourceManager::parse_full_source() {
    report_manager.reset();

    module_manager.clear();
    module_manager.load_all();

    lang::SemanticAnalyzer analyzer = lang::SemanticAnalyzer(module_manager, type_manager, nullptr);
    analyzer.enable_dependency_tracking();
    analyzer.enable_symbol_use_tracking();

    lang::SemanticAnalysis analysis = analyzer.analyze();
    report_manager.merge_result(std::move(analysis.reports), analysis.is_valid);

    is_reparse_required = false;
}

SourceFile &SourceManager::get_or_load_file(const lang::ModulePath &path, std::filesystem::path absolute_file_path) {
    if (files.count(absolute_file_path.string())) {
        return files.at(absolute_file_path.string());
    }

    std::ifstream stream(absolute_file_path, std::ios::binary | std::ios::ate);
    std::streamsize size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::string source;
    source.resize(size);
    stream.read(source.data(), size);

    SourceFile file{
        .file_path = absolute_file_path,
        .source = std::move(source),
        .module_path = path,
        .module_node = nullptr,
        .is_valid = false,
    };
    files.insert({absolute_file_path.string(), file});

    return files.at(absolute_file_path.string());
}

CompletionContext SourceManager::parse_for_completion(
    std::filesystem::path file_path,
    lang::TextPosition completion_point
) {
    const SourceFile &file = get_file(file_path);
    const lang::ModulePath &path = file.module_path;

    std::stringstream stream(file.source);
    lang::Lexer lexer(stream);
    lexer.enable_completion(completion_point);
    std::vector<lang::Token> tokens = lexer.tokenize();

    lang::Parser parser(tokens, path);
    parser.enable_completion(completion_point);
    lang::ParsedAST parsed_ast = parser.parse_module();

    lang::ModuleFile module_file = {
        .path = parsed_ast.module_->get_path(),
        .file_path = file_path,
    };

    module_manager.link_sub_modules(parsed_ast.module_, module_file);

    return CompletionContext{
        .module_node = parsed_ast.module_,
        .node = parser.get_completion_node(),
    };
}

lang::SemanticAnalysis SourceManager::analyze_isolated_module(lang::ASTModule *module_) {
    lang::SemanticAnalyzer analyzer(module_manager, type_manager, nullptr);
    return analyzer.analyze_module(module_);
}

void SourceManager::on_file_changed(SourceFile &file) {
    std::unordered_set<lang::ASTModule *> affected_modules = {file.module_node};
    walk_dependents(file.module_node, affected_modules);

    std::vector<lang::ModulePath> paths_to_analyze;
    for (lang::ASTModule *mod : affected_modules) {
        module_manager.reload(mod);
        paths_to_analyze.push_back(mod->get_path());
    }

    report_manager.reset();

    std::vector<lang::ASTModule *> modules_to_analyze;
    modules_to_analyze.reserve(paths_to_analyze.size());

    for (const lang::ModulePath &path : paths_to_analyze) {
        modules_to_analyze.push_back(module_manager.get_module_list().get_by_path(path));
    }

    lang::SemanticAnalyzer analyzer(module_manager, type_manager, nullptr);
    analyzer.enable_dependency_tracking();
    analyzer.enable_symbol_use_tracking();

    lang::SemanticAnalysis analysis = analyzer.analyze_modules(modules_to_analyze);
    report_manager.merge_result(std::move(analysis.reports), analysis.is_valid);
}

void SourceManager::walk_dependents(lang::ASTModule *mod, std::unordered_set<lang::ASTModule *> &dependents) {
    for (lang::ASTModule *dependent : mod->get_dependents()) {
        if (!dependents.contains(dependent)) {
            dependents.insert(dependent);
            walk_dependents(dependent, dependents);
        }
    }
}

SourceFile &SourceManager::get_file(std::filesystem::path path) {
    std::filesystem::path absolute_path = std::filesystem::absolute(path);
    return files.at(absolute_path.string());
}

bool SourceManager::has_file(std::filesystem::path path) {
    std::filesystem::path absolute_path = std::filesystem::absolute(path);
    return files.contains(absolute_path.string());
}

SourceFile &SourceManager::get_file(const lang::ModulePath &path) {
    auto it =
        std::find_if(files.begin(), files.end(), [path](const auto &pair) { return pair.second.module_path == path; });
    return it->second;
}

} // namespace lsp

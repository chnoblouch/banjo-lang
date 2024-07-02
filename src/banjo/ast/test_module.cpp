#include "test_module.hpp"

#include "ast/ast_utils.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#include <iostream>
#include <set>
#include <vector>

namespace banjo {

namespace lang {

ASTModule *TestModuleGenerator::generate(ModuleList &module_list, const Config &config) {
    std::vector<Function *> tests;
    std::set<std::string> used_roots;

    source << "use c.lib.string.strcmp;\n\n";

    for (ASTNode *mod_node : module_list) {
        ASTModule *mod = mod_node->as<ASTModule>();
        SymbolTable *symbol_table = ASTUtils::get_module_symbol_table(mod);

        for (Function *func : symbol_table->get_functions()) {
            if (!func->get_modifiers().test) {
                continue;
            }

            const std::string &root = mod->get_path().get(0);

            if (!used_roots.contains(root)) {
                source << "use " << root << ";\n";
                used_roots.insert(root);
            }

            tests.push_back(func);
        }
    }

    source << "\n@[exposed]\n@[link_name=main]\nfunc test_main(argc: i32, argv: **u8) -> i32 {\n";

    for (Function *test : tests) {
        std::string test_path = test->get_module()->get_path().to_string() + "." + test->get_name();
        std::cout << test_path << std::endl;

        source << "    if strcmp(argv[1], \"" << test_path << "\") == 0 {\n";
        source << "        " << test_path << "();\n";
        source << "    }\n\n";
    }

    source << "    return 0;\n";
    source << "}\n";

    std::vector<Token> tokens = Lexer(source).tokenize();
    return Parser(tokens, {"test"}).parse_module().module_;
}

} // namespace lang

} // namespace banjo

#include "test_driver_generator.hpp"

#include "banjo/sema/semantic_analyzer.hpp"

#include <iostream>

namespace banjo {

namespace lang {

TestDriverGenerator::TestDriverGenerator(sir::Unit &unit, target::Target *target, ReportManager &report_manager)
  : unit(unit),
    target(target),
    report_manager(report_manager) {}

sir::Module *TestDriverGenerator::generate() {
    sir::Module *mod = unit.create_mod();

    mod->block.symbol_table = mod->create_symbol_table(sir::SymbolTable{
        .parent = nullptr,
        .symbols{},
    });

    sir::SymbolTable *main_symbol_table = mod->create_symbol_table(sir::SymbolTable{
        .parent = mod->block.symbol_table,
        .symbols{},
    });

    std::vector<sir::MapLiteralEntry> map_entries;

    for (sir::Module *mod : unit.mods) {
        for (sir::Decl &decl : mod->block.decls) {
            if (auto func_def = decl.match<sir::FuncDef>()) {
                if (!func_def->attrs || !func_def->attrs->test) {
                    continue;
                }

                map_entries.push_back(sir::MapLiteralEntry{
                    .key = mod->create_expr(sir::StringLiteral{
                        .ast_node = nullptr,
                        .type = nullptr,
                        .value = func_def->ident.value,
                    }),
                    .value = mod->create_trivial(sir::SymbolExpr{
                        .ast_node = nullptr,
                        .type = &func_def->type,
                        .symbol = func_def,
                    }),
                });

                std::cout << func_def->ident.value << '\n';
            }
        }
    }

    std::vector<sir::Stmt> stmts;

    if (!map_entries.empty()) {
        sir::VarStmt *map_var_stmt = mod->create_stmt(sir::VarStmt{
            .ast_node = nullptr,
            .local{
                .name{
                    .ast_node = nullptr,
                    .value = "map",
                },
                .type = nullptr,
            },
            .value = mod->create_expr(sir::MapLiteral{
                .ast_node = nullptr,
                .type = nullptr,
                .entries = map_entries,
            }),
        });

        sir::Expr test_name = mod->create_expr(sir::CallExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .callee = mod->create_expr(sir::DotExpr{
                .ast_node = nullptr,
                .lhs = mod->create_expr(sir::IdentExpr{
                    .ast_node = nullptr,
                    .value = "String",
                }),
                .rhs{
                    .ast_node = nullptr,
                    .value = "from_cstr",
                }
            }),
            .args{
                mod->create_expr(sir::BracketExpr{
                    .ast_node = nullptr,
                    .lhs = mod->create_expr(sir::IdentExpr{
                        .ast_node = nullptr,
                        .value = "argv",
                    }),
                    .rhs{
                        mod->create_trivial(sir::IntLiteral{
                            .ast_node = nullptr,
                            .type = nullptr,
                            .value = 1,
                        }),
                    },
                }),
            },
        });

        sir::Expr test_call_expr = mod->create_expr(sir::CallExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .callee = mod->create_expr(sir::IdentExpr{
                .ast_node = nullptr,
                .value = "function",
            }),
            .args{},
        });

        sir::Expr error_call_expr = mod->create_expr(sir::CallExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .callee = mod->create_expr(sir::IdentExpr{
                .ast_node = nullptr,
                .value = "println",
            }),
            .args{
                mod->create_expr(sir::StringLiteral{
                    .ast_node = nullptr,
                    .type = nullptr,
                    .value = "no such test",
                }),
            },
        });

        sir::TryStmt *try_stmt = mod->create_stmt(sir::TryStmt{
            .ast_node = nullptr,
            .success_branch{
                .ast_node = nullptr,
                .ident{
                    .ast_node = nullptr,
                    .value = "function",
                },
                .expr = mod->create_expr(sir::CallExpr{
                    .ast_node = nullptr,
                    .type = nullptr,
                    .callee = mod->create_expr(sir::DotExpr{
                        .ast_node = nullptr,
                        .lhs = mod->create_expr(sir::IdentExpr{
                            .ast_node = nullptr,
                            .value = "map",
                        }),
                        .rhs{
                            .ast_node = nullptr,
                            .value = "try_get",
                        },
                    }),
                    .args{test_name},
                }),
                .block{
                    .ast_node = nullptr,
                    .stmts{
                        mod->create_stmt(test_call_expr),
                    },
                    .symbol_table = mod->create_symbol_table(sir::SymbolTable{
                        .parent = main_symbol_table,
                        .symbols{},
                    }),
                }
            },
            .except_branch{},
            .else_branch{sir::TryElseBranch{
                .ast_node = nullptr,
                .block{
                    .ast_node = nullptr,
                    .stmts{
                        mod->create_stmt(error_call_expr),
                    },
                    .symbol_table = mod->create_symbol_table(sir::SymbolTable{
                        .parent = main_symbol_table,
                        .symbols{},
                    }),
                }
            }}
        });

        stmts = {map_var_stmt, try_stmt};
    }

    sir::FuncDef *main_def = mod->create_decl(sir::FuncDef{
        .ast_node = nullptr,
        .ident{
            .ast_node = nullptr,
            .value = "main",
        },
        .type{
            .ast_node = nullptr,
            .params{
                sir::Param{
                    .ast_node = nullptr,
                    .name{
                        .ast_node = nullptr,
                        .value = "argc",
                    },
                    .type = mod->create_trivial(sir::PrimitiveType{
                        .ast_node = nullptr,
                        .primitive = sir::Primitive::I32,
                    }),
                },
                sir::Param{
                    .ast_node = nullptr,
                    .name{
                        .ast_node = nullptr,
                        .value = "argv",
                    },
                    .type = mod->create_trivial(sir::PointerType{
                        .ast_node = nullptr,
                        .base_type = mod->create_trivial(sir::PointerType{
                            .ast_node = nullptr,
                            .base_type = mod->create_trivial(sir::PrimitiveType{
                                .ast_node = nullptr,
                                .primitive = sir::Primitive::U8,
                            }),
                        })
                    }),
                }
            },
            .return_type = mod->create_trivial(sir::PrimitiveType{
                .ast_node = nullptr,
                .primitive = sir::Primitive::VOID,
            }),
        },
        .block{
            .ast_node = nullptr,
            .stmts = stmts,
            .symbol_table = main_symbol_table,
        },
        .attrs = mod->create_attrs(sir::Attributes{
            .link_name = "main",
        }),
    });

    mod->block.decls.push_back(main_def);

    sema::SemanticAnalyzer(unit, target, report_manager).analyze(*mod);

    return mod;
}

} // namespace lang

} // namespace banjo

#ifndef BANJO_SSA_GEN_SPECIALIZATION_COLLECTOR_H
#define BANJO_SSA_GEN_SPECIALIZATION_COLLECTOR_H

#include "banjo/sir/sir.hpp"

#include <unordered_map>
#include <vector>

namespace banjo::lang {

struct Specialization {};

class SpecializationCollector {

public:
    struct Entry {
        const std::vector<sir::GenericParam> &params;
        std::vector<sir::Expr> args;
    };

public:
    typedef std::unordered_map<sir::Symbol, std::vector<Entry>> Map;

private:
    Map specializations;
    std::vector<Entry> entry_stack;

public:
    Map collect(const sir::Unit &unit);

private:
    void visit_decl_block(const sir::DeclBlock &decl_block);
    void visit_func_def(const sir::FuncDef &func_def);

    void visit_block(const sir::Block &block);
    void visit_stmt(sir::Stmt stmt);
    void visit_var_stmt(const sir::VarStmt &var_stmt);
    void visit_assign_stmt(const sir::AssignStmt &assign_stmt);
    void visit_return_stmt(const sir::ReturnStmt &return_stmt);
    void visit_if_stmt(const sir::IfStmt &if_stmt);

    void visit_expr(sir::Expr expr);
    void visit_call_expr(const sir::CallExpr &call_expr);
    void visit_specialize_expr(const sir::SpecializeExpr &specialize_expr);
};

} // namespace banjo::lang

#endif

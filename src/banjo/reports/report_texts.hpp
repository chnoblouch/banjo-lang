#ifndef BANJO_REPORTS_REPORT_TEXTS_H
#define BANJO_REPORTS_REPORT_TEXTS_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/module_path.hpp"

#include <string>
#include <string_view>

namespace banjo {

namespace lang {

class ReportText {

private:
    std::string text;

public:
    ReportText(std::string_view format_str);
    std::string str() { return text; }

    ReportText &format(std::string_view string);
    ReportText &format(const char *string);
    ReportText &format(const std::string &string);
    ReportText &format(int integer);
    ReportText &format(unsigned integer);
    ReportText &format(long integer);
    ReportText &format(unsigned long integer);
    ReportText &format(long long integer);
    ReportText &format(unsigned long long integer);
    ReportText &format(LargeInt integer);
    ReportText &format(ASTNode *node);
    ReportText &format(const ModulePath &path);
    ReportText &format(sir::Expr &expr);
    ReportText &format(sir::ExprCategory expr_category);
    ReportText &format(const std::vector<sir::Expr> &exprs);
    ReportText &format(const std::vector<sir::GenericParam> &generic_params);

    static std::string to_string(const sir::Expr &expr);

private:
    static std::string func_type_to_string(const sir::FuncType &func_type);
    static std::string symbol_to_string(sir::Symbol symbol);
    static std::string tuple_expr_to_string(const sir::TupleExpr &tuple_expr);
    static std::string primitive_to_string(sir::Primitive primitive);
    static std::string closure_type_to_string(const sir::ClosureType &closure_type);
    static std::string reference_type_to_string(const sir::ReferenceType &reference_type);
    static std::string pseudo_type_to_string(sir::PseudoTypeKind pseudo_type);
};

} // namespace lang

} // namespace banjo

#endif

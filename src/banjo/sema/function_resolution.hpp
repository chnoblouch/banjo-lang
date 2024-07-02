#ifndef FUNCTION_RESOLUTION
#define FUNCTION_RESOLUTION

#include "banjo/sema/location_analyzer.hpp"
#include "banjo/sema/semantic_analyzer_context.hpp"
#include "banjo/symbol/symbol_ref.hpp"

#include <optional>

namespace banjo {

namespace lang {

class FunctionResolution {

private:
    SemanticAnalyzerContext &context;
    ASTNode *node;
    const SymbolRef &symbol;
    SymbolUsage usage;

public:
    FunctionResolution(SemanticAnalyzerContext &context, ASTNode *node, const SymbolRef &symbol, SymbolUsage usage);
    SymbolRef resolve();
    std::optional<std::vector<DataType *>> analyze_arg_types();

private:
    SymbolRef resolve_func();
    SymbolRef resolve_overload();
    SymbolRef resolve_generic_func();

    void report_unresolved_overload(std::vector<DataType *> &arg_types);
};

} // namespace lang

} // namespace banjo

#endif

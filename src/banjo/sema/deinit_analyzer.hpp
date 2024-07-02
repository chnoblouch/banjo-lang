#ifndef DEINIT_ANALYZER_H
#define DEINIT_ANALYZER_H

#include "ast/ast_block.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "symbol/local_variable.hpp"
#include "symbol/location.hpp"
#include "symbol/parameter.hpp"

namespace banjo {

namespace lang {

class DeinitAnalyzer {

public:
    DeinitAnalyzer(SemanticAnalyzerContext &context);
    void analyze_local(ASTBlock *block, LocalVariable *local);
    void analyze_param(ASTBlock *block, Parameter *param);

private:
    void analyze_var(ASTBlock *block, Variable *var, LocationElement location_element);

    void create_struct_info(ASTBlock *block, Variable *var, LocationElement location_element);
    void create_member_info(DeinitInfo &info, StructField *field);
    void create_union_info(ASTBlock *block, Variable* var, LocationElement location_element);

    void register_info_on_block(DeinitInfo &info, ASTBlock *block);
};

} // namespace lang

} // namespace banjo

#endif

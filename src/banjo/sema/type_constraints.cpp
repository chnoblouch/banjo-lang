#include "type_constraints.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo::lang::sema {

Result check_type_constraint(
    SemanticAnalyzer &analyzer,
    ASTNode *ast_node,
    std::span<sir::GenericParam *> params,
    std::span<sir::Expr> args,
    unsigned index
) {
    sir::GenericParam &param = *params[index];
    sir::Expr arg = args[index];

    if (!param.constraint) {
        return Result::SUCCESS;
    }

    sir::Concrete<sir::ProtoDef> concrete_proto = param.constraint.as_concrete<sir::ProtoDef>();

    utils::Arena arena;
    sir::Specializer specializer{arena, params, args};
    concrete_proto.generic_args = specializer.specialize_expr_list(concrete_proto.generic_args);

    bool satisfied = false;

    if (auto primitive_type = arg.match<sir::PrimitiveType>()) {
        satisfied = primitive_implements(analyzer, primitive_type->primitive, concrete_proto);
    } else if (auto struct_def = arg.match_symbol<sir::StructDef>()) {
        satisfied = struct_def->has_impl_for(concrete_proto);
    } else if (auto other_param = arg.match_symbol<sir::GenericParam>()) {
        if (auto other_proto = other_param->constraint.match_concrete<sir::ProtoDef>()) {
            satisfied = concrete_proto == other_proto;
        }
    }

    if (satisfied) {
        return Result::SUCCESS;
    } else {
        analyzer.report_generator.report_err_constraint_not_satisfied(ast_node, arg, param);
        return Result::ERROR;
    }
}

bool primitive_implements(
    SemanticAnalyzer &analyzer,
    sir::Primitive primitive,
    sir::Concrete<sir::ProtoDef> proto_def
) {
    if (proto_def.def == analyzer.std_compare_def) {
        if (primitive == sir::Primitive::VOID) {
            return false;
        }

        return proto_def.generic_args[0].is_primitive_type(primitive);
    } else if (proto_def.def == analyzer.std_order_def) {
        switch (primitive) {
            case sir::Primitive::I8:
            case sir::Primitive::I16:
            case sir::Primitive::I32:
            case sir::Primitive::I64:
            case sir::Primitive::U8:
            case sir::Primitive::U16:
            case sir::Primitive::U32:
            case sir::Primitive::U64:
            case sir::Primitive::USIZE:
            case sir::Primitive::F32:
            case sir::Primitive::F64: return proto_def.generic_args[0].is_primitive_type(primitive);
            case sir::Primitive::BOOL:
            case sir::Primitive::ADDR:
            case sir::Primitive::VOID: return false;
        }
    } else {
        return false;
    }
}

} // namespace banjo::lang::sema

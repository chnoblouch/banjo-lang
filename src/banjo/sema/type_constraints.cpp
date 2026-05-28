#include "type_constraints.hpp"
#include "banjo/sir/resource_generator.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/utils/arena.hpp"
#include "banjo/utils/utils.hpp"

#include <initializer_list>

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

    if (!satisfied && concrete_proto.def == analyzer.std_copy_def) {
        utils::Arena arena;
        satisfied = !sir::ResourceGenerator{arena}.create_resource(arg).has_value();
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
    std::initializer_list<sir::ProtoDef *> numeric_protos{
        analyzer.std_order_def,
        analyzer.std_add_def,
        analyzer.std_sub_def,
        analyzer.std_mul_def,
        analyzer.std_div_def,
    };

    std::initializer_list<sir::ProtoDef *> int_protos{
        analyzer.std_mod_def,
        analyzer.std_bit_and_def,
        analyzer.std_bit_or_def,
        analyzer.std_bit_xor_def,
        analyzer.std_shl_def,
        analyzer.std_shr_def,
    };

    if (proto_def.def == analyzer.std_compare_def) {
        if (primitive == sir::Primitive::VOID) {
            return false;
        }

        return proto_def.generic_args[0].is_primitive_type(primitive);
    } else if (Utils::is_one_of(proto_def.def, numeric_protos)) {
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
    } else if (Utils::is_one_of(proto_def.def, int_protos)) {
        switch (primitive) {
            case sir::Primitive::I8:
            case sir::Primitive::I16:
            case sir::Primitive::I32:
            case sir::Primitive::I64:
            case sir::Primitive::U8:
            case sir::Primitive::U16:
            case sir::Primitive::U32:
            case sir::Primitive::U64:
            case sir::Primitive::USIZE: return proto_def.generic_args[0].is_primitive_type(primitive);
            case sir::Primitive::F32:
            case sir::Primitive::F64:
            case sir::Primitive::BOOL:
            case sir::Primitive::ADDR:
            case sir::Primitive::VOID: return false;
        }
    } else {
        return false;
    }
}

} // namespace banjo::lang::sema

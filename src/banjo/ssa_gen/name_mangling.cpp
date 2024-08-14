#include "name_mangling.hpp"

#include <ranges>

namespace banjo {

namespace lang {

namespace NameMangling {

std::string get_link_name(SSAGeneratorContext &ctx, const sir::FuncDef &func) {
    if (func.attrs && func.attrs->link_name) {
        return *func.attrs->link_name;
    } else if (func.is_main() || (func.attrs && (func.attrs->exposed || func.attrs->dllexport))) {
        return func.ident.value;
    } else {
        return mangle_func_name(ctx, func);
    }
}

const std::string &get_link_name(const sir::NativeFuncDecl &func) {
    if (func.attrs && func.attrs->link_name) {
        return *func.attrs->link_name;
    } else {
        return func.ident.value;
    }
}

static void mangle_type(std::string &string, const sir::Expr &type) {
    if (auto primitive_type = type.match<sir::PrimitiveType>()) {
        string += 'p';

        switch (primitive_type->primitive) {
            case sir::Primitive::I8: string += "i0"; break;
            case sir::Primitive::I16: string += "i1"; break;
            case sir::Primitive::I32: string += "i2"; break;
            case sir::Primitive::I64: string += "i3"; break;
            case sir::Primitive::U8: string += "u0"; break;
            case sir::Primitive::U16: string += "u1"; break;
            case sir::Primitive::U32: string += "u2"; break;
            case sir::Primitive::U64: string += "u3"; break;
            case sir::Primitive::F32: string += "f0"; break;
            case sir::Primitive::F64: string += "f1"; break;
            case sir::Primitive::BOOL: string += "b0"; break;
            case sir::Primitive::ADDR: string += "a0"; break;
            case sir::Primitive::VOID: string += "v0"; break;
        }
    } else if (auto symbol_expr = type.match<sir::SymbolExpr>()) {
        string += 's';
    }
}

std::string mangle_func_name(SSAGeneratorContext &ctx, const sir::FuncDef &func) {
    std::string string = "bnj_";

    for (const SSAGeneratorContext::DeclContext &decl_ctx : std::ranges::views::reverse(ctx.get_decl_contexts())) {
        if (decl_ctx.sir_mod) {
            for (const std::string &path_component : decl_ctx.sir_mod->path.get_path()) {
                string += 'm';
                string += std::to_string(path_component.size());
                string += path_component;
            }
        } else if (decl_ctx.sir_struct_def) {
            string += 's';
            string += std::to_string(decl_ctx.sir_struct_def->ident.value.size());
            string += decl_ctx.sir_struct_def->ident.value;

            if (decl_ctx.sir_generic_args) {
                string += 'g';

                for (const sir::Expr &generic_arg : *decl_ctx.sir_generic_args) {
                    mangle_type(string, generic_arg);
                }
            }
        }
    }

    string += 'n';
    string += std::to_string(func.ident.value.size());
    string += func.ident.value;
    string += '_';

    for (const sir::Param &param : func.type.params) {
        mangle_type(string, param.type);
    }

    return string;
}

} // namespace NameMangling

} // namespace lang

} // namespace banjo

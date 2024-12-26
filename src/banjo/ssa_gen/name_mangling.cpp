#include "name_mangling.hpp"

#include "banjo/config/config.hpp"

#include <cstdlib>
#include <string>

namespace banjo {

namespace lang {

namespace NameMangling {

static void mangle_symbol_name(std::string &string, sir::Symbol symbol);

std::string get_link_name(const sir::FuncDef &func) {
    if (func.attrs && func.attrs->link_name) {
        return *func.attrs->link_name;
    } else if (func.is_main() && !Config::instance().testing) {
        return "main";
    } else if (func.attrs && (func.attrs->exposed || func.attrs->dllexport)) {
        return func.ident.value;
    } else {
        return mangle_func_name(func);
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
            case sir::Primitive::USIZE: string += "u4"; break;
            case sir::Primitive::F32: string += "f0"; break;
            case sir::Primitive::F64: string += "f1"; break;
            case sir::Primitive::BOOL: string += "b0"; break;
            case sir::Primitive::ADDR: string += "a0"; break;
            case sir::Primitive::VOID: string += "v0"; break;
        }
    } else if (auto symbol_expr = type.match<sir::SymbolExpr>()) {
        string += 's';
        mangle_symbol_name(string, symbol_expr->symbol);
    } else if (auto pointer_type = type.match<sir::PointerType>()) {
        string += 'a';
        mangle_type(string, pointer_type->base_type);
    } else if (auto tuple_type = type.match<sir::TupleExpr>()) {
        string += 't';
        string += std::to_string(tuple_type->exprs.size());

        for (sir::Expr type : tuple_type->exprs) {
            mangle_type(string, type);
        }
    }
}

static void mangle_symbol_name(std::string &string, sir::Symbol symbol) {
    if (!symbol) {
        return;
    }

    mangle_symbol_name(string, symbol.get_parent());

    if (auto mod = symbol.match<sir::Module>()) {
        for (const std::string &path_component : mod->path.get_path()) {
            string += 'm';
            string += std::to_string(path_component.size());
            string += path_component;
        }
    } else if (auto struct_def = symbol.match<sir::StructDef>()) {
        string += 's';
        string += std::to_string(struct_def->ident.value.size());
        string += struct_def->ident.value;

        if (struct_def->parent_specialization) {
            string += 'g';

            for (const sir::Expr &generic_arg : struct_def->parent_specialization->args) {
                mangle_type(string, generic_arg);
            }
        }
    }
}

std::string mangle_func_name(const sir::FuncDef &func) {
    std::string string = "bnj_";

    mangle_symbol_name(string, func.parent);

    string += 'n';
    string += std::to_string(func.ident.value.size());
    string += func.ident.value;

    if (func.parent_specialization) {
        string += 'g';

        for (const sir::Expr &generic_arg : func.parent_specialization->args) {
            mangle_type(string, generic_arg);
        }
    }

    string += 'p';

    for (const sir::Param &param : func.type.params) {
        mangle_type(string, param.type);
    }

    if (func.parent) {
        const sir::SymbolTable &parent_symbol_table = *func.parent.get_symbol_table();
        sir::Symbol symbol = parent_symbol_table.look_up_local(func.ident.value);

        if (auto overload_set = symbol.match<sir::OverloadSet>()) {
            for (unsigned i = 0; i < overload_set->func_defs.size(); i++) {
                if (&func == overload_set->func_defs[i]) {
                    string += "_overload" + std::to_string(i);
                    break;
                }
            }
        }
    }

    string += std::to_string(std::rand());

    return string;
}

} // namespace NameMangling

} // namespace lang

} // namespace banjo

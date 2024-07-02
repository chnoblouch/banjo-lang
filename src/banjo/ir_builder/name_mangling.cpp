#include "name_mangling.hpp"

#include "banjo/ast/ast_module.hpp"
#include "banjo/symbol/protocol.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/symbol/union.hpp"

#include <string>

namespace banjo {

namespace ir_builder {

std::string NameMangling::mangle_mod_path(const lang::ModulePath &mod_path) {
    return mod_path.to_string(".");
}

std::string NameMangling::mangle_func_name(lang::Function *func) {
    if (func->is_native()) {
        return func->get_name();
    } else {
        const lang::ModulePath &module_path = func->get_module()->get_path();
        std::string result = "func." + mangle_mod_path(module_path) + ".";

        const lang::SymbolRef &enclosing_symbol = func->get_enclosing_symbol();
        if (enclosing_symbol.get_kind() == lang::SymbolKind::STRUCT) {
            result += enclosing_symbol.get_struct()->get_name() + ".";
        } else if (enclosing_symbol.get_kind() == lang::SymbolKind::UNION) {
            result += enclosing_symbol.get_union()->get_name() + ".";
        }

        result += func->get_name() + "$";

        for (lang::DataType *param : func->get_type().param_types) {
            result += mangle_type_name(param);
            if (param != func->get_type().param_types.back()) {
                result += "$";
            }
        }

        return result;
    }
}

std::string NameMangling::mangle_generic_func_name(lang::Function *func, const lang::GenericInstanceArgs &args) {
    std::string result = mangle_func_name(func) + ".";

    /*
    for(ASTNode* arg : args) {
        result += mangle_type_name(type);
        if(type != instance.back()) {
            result += "$";
        }
    }
    */

    return result;
}

std::string NameMangling::mangle_global_var_name(lang::GlobalVariable *global_var) {
    if (global_var->is_native()) {
        return global_var->get_name();
    } else {
        return mangle_mod_path(global_var->get_module()->get_path()) + "." + global_var->get_name();
    }
}

std::string NameMangling::mangle_type_name(lang::DataType *type) {
    if (type->get_kind() == lang::DataType::Kind::PRIMITIVE) {
        switch (type->get_primitive_type()) {
            case lang::PrimitiveType::I8: return "i8";
            case lang::PrimitiveType::I16: return "i16";
            case lang::PrimitiveType::I32: return "i32";
            case lang::PrimitiveType::I64: return "i64";
            case lang::PrimitiveType::U8: return "u8";
            case lang::PrimitiveType::U16: return "u16";
            case lang::PrimitiveType::U32: return "u32";
            case lang::PrimitiveType::U64: return "u64";
            case lang::PrimitiveType::F32: return "f32";
            case lang::PrimitiveType::F64: return "f64";
            case lang::PrimitiveType::BOOL: return "bool";
            case lang::PrimitiveType::ADDR: return "addr";
            case lang::PrimitiveType::VOID: return "void";
        }
    } else if (type->get_kind() == lang::DataType::Kind::STRUCT) {
        return "s" + mangle_struct_name(type->get_structure());
    } else if (type->get_kind() == lang::DataType::Kind::POINTER) {
        return "p" + mangle_type_name(type->get_base_data_type());
    } else if (type->get_kind() == lang::DataType::Kind::FUNCTION) {
        return "f";
    } else if (type->get_kind() == lang::DataType::Kind::CLOSURE) {
        return "c";
    } else {
        return "INVALID_TYPE";
    }
}

std::string NameMangling::mangle_struct_name(lang::Structure *struct_) {
    const lang::ModulePath &module_path = struct_->get_module()->get_path();
    return mangle_mod_path(module_path) + "." + struct_->get_name();
}

std::string NameMangling::mangle_union_name(lang::Union *union_) {
    const lang::ModulePath &module_path = union_->get_module()->get_path();
    return mangle_mod_path(module_path) + "." + union_->get_name();
}

std::string NameMangling::mangle_union_case_name(lang::UnionCase *case_) {
    return mangle_union_name(case_->get_union()) + "." + case_->get_name();
}

std::string NameMangling::mangle_proto_name(lang::Protocol *proto) {
    // const lang::ModulePath &module_path = union_->get_module()->get_path();
    // return mangle_mod_path(module_path) + "." + union_->get_name();

    return proto->get_name();
}

std::string NameMangling::mangle_generic_struct_name(
    lang::Structure *struct_,
    const lang::GenericInstanceArgs &instance
) {
    std::string result = mangle_struct_name(struct_) + ".";

    /*
    for(lang::DataType* type : instance) {
        result += mangle_type_name(type);
        if(type != instance.back()) {
            result += "@";
        }
    }
    */

    return result;
}

} // namespace ir_builder

} // namespace banjo

#include "report_utils.hpp"

#include "symbol/enumeration.hpp"
#include "symbol/structure.hpp"
#include "symbol/union.hpp"
#include "symbol/protocol.hpp"

namespace lang {

std::string ReportUtils::type_to_string(DataType *type) {
    if (type->get_kind() == DataType::Kind::PRIMITIVE) {
        switch (type->get_primitive_type()) {
            case I8: return "i8";
            case I16: return "i16";
            case I32: return "i32";
            case I64: return "i64";
            case U8: return "u8";
            case U16: return "u16";
            case U32: return "u32";
            case U64: return "u64";
            case F32: return "f32";
            case F64: return "f64";
            case BOOL: return "bool";
            case ADDR: return "addr";
            case VOID: return "void";
        }
    } else if (type->get_kind() == DataType::Kind::ARRAY) {
        return "[" + type_to_string(type->get_base_data_type()) + "]";
    } else if (type->get_kind() == DataType::Kind::POINTER) {
        return "*" + type_to_string(type->get_base_data_type());
    } else if (type->get_kind() == DataType::Kind::STRUCT) {
        return struct_to_string(type->get_structure());
    } else if (type->get_kind() == DataType::Kind::ENUM) {
        return enum_to_string(type->get_enumeration());
    } else if (type->get_kind() == DataType::Kind::UNION) {
        return union_to_string(type->get_union());
    } else if (type->get_kind() == DataType::Kind::UNION_CASE) {
        return type->get_union_case()->get_name();
    } else if (type->get_kind() == DataType::Kind::PROTO) {
        return type->get_protocol()->get_name();
    } else if (type->get_kind() == DataType::Kind::FUNCTION) {
        const FunctionType &func_type = type->get_function_type();

        std::string params_str = "";
        for (unsigned int i = 0; i < func_type.param_types.size(); i++) {
            params_str += type_to_string(func_type.param_types[i]);
            if (i != func_type.param_types.size() - 1) {
                params_str += ", ";
            }
        }

        return "func(" + params_str + ") -> " + type_to_string(func_type.return_type);
    } else if (type->get_kind() == DataType::Kind::STATIC_ARRAY) {
        DataType *base_type = type->get_static_array_type().base_type;
        unsigned length = type->get_static_array_type().length;
        return "[" + type_to_string(base_type) + "; " + std::to_string(length) + "]";
    } else if (type->get_kind() == DataType::Kind::TUPLE) {
        const Tuple &tuple = type->get_tuple();

        std::string types_str = "";
        for (unsigned int i = 0; i < tuple.types.size(); i++) {
            types_str += type_to_string(tuple.types[i]);
            if (i != tuple.types.size() - 1) {
                types_str += ", ";
            }
        }

        return "(" + types_str + ")";
    } else if (type->get_kind() == DataType::Kind::CLOSURE) {
        const FunctionType &func_type = type->get_function_type();

        std::string params_str = "";
        for (unsigned int i = 0; i < func_type.param_types.size(); i++) {
            params_str += type_to_string(func_type.param_types[i]);
            if (i != func_type.param_types.size() - 1) {
                params_str += ", ";
            }
        }

        return "|" + params_str + "| -> " + type_to_string(func_type.return_type);
    }

    return "???";
}

std::string ReportUtils::struct_to_string(Structure *struct_) {
    // TODO: only care about the module path if there are mixed paths?
    /*
    const ModulePath& module_path = struct_->get_module()->get_path();
    return module_path.to_string(".") + "." + struct_->get_name();
    */

    return struct_->get_name();
}

std::string ReportUtils::enum_to_string(Enumeration *enum_) {
    // TODO: only care about the module path if there are mixed paths?
    return enum_->get_name();
}

std::string ReportUtils::union_to_string(Union *union_) {
    // TODO: only care about the module path if there are mixed paths?
    return union_->get_name();
}

std::string ReportUtils::params_to_string(std::vector<DataType *> &params) {
    std::string string;

    for (unsigned int i = 0; i < params.size(); i++) {
        string += type_to_string(params[i]);

        if (i != params.size() - 1) {
            string += ", ";
        }
    }

    return string;
}

} // namespace lang

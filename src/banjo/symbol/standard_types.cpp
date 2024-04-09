#include "standard_types.hpp"

#include "ast/ast_module.hpp"
#include "symbol/structure.hpp"

#include <cassert>
#include <tuple>
#include <vector>

namespace lang {

bool StandardTypes::is_optional(DataType *type) {
    return is_struct(type, {"std", "optional"}, "Optional");
}

bool StandardTypes::is_result(DataType *type) {
    return is_struct(type, {"std", "result"}, "Result");
}

bool StandardTypes::is_string(DataType *type) {
    return is_struct(type, {"std", "string"}, "String");
}

DataType *StandardTypes::get_array_base_type(DataType *array_type) {
    return get_struct_field_type(array_type, "data")->get_base_data_type();
}

DataType *StandardTypes::get_optional_value_type(DataType *optional_type) {
    return get_struct_field_type(optional_type, "value");
}

DataType *StandardTypes::get_result_value_type(DataType *result_type) {
    return get_struct_field_type(result_type, "value");
}

DataType *StandardTypes::get_result_error_type(DataType *result_type) {
    return get_struct_field_type(result_type, "error");
}

bool StandardTypes::is_struct(DataType *type, const ModulePath &module_path, const std::string &name) {
    if (!type || type->get_kind() != DataType::Kind::STRUCT) {
        return false;
    }

    Structure *struct_ = type->get_structure();
    const ModulePath &actual_path = struct_->get_module()->get_path();
    const std::string &actual_name = struct_->get_name();
    return actual_name.starts_with(name) && actual_path == module_path;
}

DataType *StandardTypes::get_struct_field_type(DataType *type, const std::string &field_name) {
    Structure *struct_ = type->get_structure();
    StructField *field = struct_->get_field(field_name);
    assert(field);
    return field->get_type();
}

void StandardTypes::link_symbols(SymbolTable *symbol_table, const ModuleList &module_list) {
    std::vector<std::tuple<ModulePath, std::string>> symbols = {
        {{"banjo"}, "print"},
        {{"banjo"}, "println"},
        {{"std", "system"}, "panic"},
        {{"banjo"}, "assert"},
        {{"std", "array"}, "Array"},
        {{"std", "map"}, "Map"},
        {{"std", "set"}, "Set"},
        {{"std", "optional"}, "Optional"},
        {{"std", "string"}, "String"},
        {{"std", "result"}, "Result"},
        {{"std", "slice"}, "Slice"},
        {{"std", "string_slice"}, "StringSlice"},
    };

    for (const auto &[path, name] : symbols) {
        ASTModule *module_ = module_list.get_by_path(path);
        if (!module_) {
            continue;
        }

        std::optional<SymbolRef> symbol = module_->get_block()->get_symbol_table()->get_symbol(name);
        if (!symbol) {
            continue;
        }

        symbol_table->add_symbol(name, symbol->as_link());
    }
}

} // namespace lang

#ifndef STANDARD_TYPES_H
#define STANDARD_TYPES_H

#include "ast/module_list.hpp"
#include "symbol/data_type.hpp"
#include "symbol/module_path.hpp"
#include "symbol/symbol_table.hpp"

namespace banjo {

namespace lang {

namespace StandardTypes {

bool is_optional(DataType *type);
bool is_result(DataType *type);
bool is_string(DataType *type);

DataType *get_array_base_type(DataType *array_type);
DataType *get_optional_value_type(DataType *optional_type);
DataType *get_result_value_type(DataType *result_type);
DataType *get_result_error_type(DataType *result_type);

bool is_struct(DataType *type, const ModulePath &module_path, const std::string &name);
DataType *get_struct_field_type(DataType *type, const std::string &field_name);

void link_symbols(SymbolTable *symbol_table, const ModuleList &module_list);

} // namespace StandardTypes

} // namespace lang

} // namespace banjo

#endif

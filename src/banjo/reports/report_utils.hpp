#ifndef REPORT_UTILS_H
#define REPORT_UTILS_H

#include "banjo/symbol/data_type.hpp"

#include <string>
#include <vector>

namespace banjo {

namespace lang {

namespace ReportUtils {

std::string type_to_string(DataType *type);
std::string struct_to_string(Structure *struct_);
std::string enum_to_string(Enumeration *enum_);
std::string union_to_string(Union *union_);
std::string params_to_string(std::vector<DataType *> &params);

} // namespace ReportUtils

} // namespace lang

} // namespace banjo

#endif

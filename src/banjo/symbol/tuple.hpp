#ifndef TUPLE_H
#define TUPLE_H

#include <vector>

namespace lang {

class DataType;

struct Tuple {
    std::vector<DataType *> types;

    bool is_static_array();
};

} // namespace lang

#endif

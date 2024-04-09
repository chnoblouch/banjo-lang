#ifndef DATA_TYPE_MANAGER
#define DATA_TYPE_MANAGER

#include "symbol/data_type.hpp"
#include <vector>

namespace lang {

class DataTypeManager {

private:
    constexpr static unsigned NUM_PRIMITIVES = 13;

    std::vector<DataType *> data_types;
    DataType primitive_types[NUM_PRIMITIVES];

public:
    DataTypeManager();
    ~DataTypeManager();
    DataType *new_data_type();
    DataType *get_primitive_type(PrimitiveType primitive_type);
};

} // namespace lang

#endif

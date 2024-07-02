#ifndef DATA_TYPE_MANAGER
#define DATA_TYPE_MANAGER

#include "banjo/symbol/data_type.hpp"
#include "banjo/utils/growable_arena.hpp"

namespace banjo {

namespace lang {

class DataTypeManager {

private:
    constexpr static unsigned NUM_PRIMITIVES = 13;

    utils::GrowableArena<DataType> data_types;
    DataType primitive_types[NUM_PRIMITIVES];

public:
    DataTypeManager();
    DataType *new_data_type();
    DataType *get_primitive_type(PrimitiveType primitive_type);
};

} // namespace lang

} // namespace banjo

#endif

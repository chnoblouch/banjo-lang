#include "data_type_manager.hpp"

namespace lang {

DataTypeManager::DataTypeManager() {
    for (unsigned i = 0; i < NUM_PRIMITIVES; i++) {
        primitive_types[i].set_to_primitive(static_cast<PrimitiveType>(i));
    }
}

DataType *DataTypeManager::new_data_type() {
    return data_types.create();
}

DataType *DataTypeManager::get_primitive_type(PrimitiveType primitive_type) {
    return &primitive_types[static_cast<unsigned>(primitive_type)];
}

} // namespace lang

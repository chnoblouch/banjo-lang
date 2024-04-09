#include "data_type_manager.hpp"
#include "symbol/data_type.hpp"

#include <iostream>
#include <unordered_map>

namespace lang {

DataTypeManager::DataTypeManager() {
    for (unsigned i = 0; i < NUM_PRIMITIVES; i++) {
        primitive_types[i].set_to_primitive(static_cast<PrimitiveType>(i));
    }
}

DataTypeManager::~DataTypeManager() {
    std::unordered_map<DataType::Kind, unsigned> counters;

    for (DataType *data_type : data_types) {
        counters[data_type->get_kind()] += 1;
        delete data_type;
    }

    // std::cout << data_types.size() << " types\n";

    // for (auto [kind, counter] : counters) {
    //     std::cout << (unsigned)kind << ": " << counter << "\n";
    // }
}

DataType *DataTypeManager::new_data_type() {
    DataType *data_type = new DataType();
    data_types.push_back(data_type);
    return data_type;
}

DataType *DataTypeManager::get_primitive_type(PrimitiveType primitive_type) {
    return &primitive_types[static_cast<unsigned>(primitive_type)];
}

} // namespace lang

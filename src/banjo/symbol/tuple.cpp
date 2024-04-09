#include "tuple.hpp"

#include "symbol/data_type.hpp"

namespace lang {

bool Tuple::is_static_array() {
    if (types.size() < 2) {
        return true;
    }

    for (int i = 1; i < types.size(); i++) {
        if (!types[i]->equals(types[0])) {
            return false;
        }
    }

    return true;
}

} // namespace lang

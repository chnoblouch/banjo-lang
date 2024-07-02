#include "utils.hpp"

namespace banjo {

namespace Utils {

int align(int value, int boundary) {
    if (boundary == 0) {
        return value;
    }

    int mod = value % boundary;
    return mod == 0 ? value : value + boundary - mod;
}

} // namespace Utils

} // namespace banjo

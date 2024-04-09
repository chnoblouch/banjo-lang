#include "standard_lib.hpp"

#include "utils/paths.hpp"

namespace lang {

StandardLib &StandardLib::instance() {
    static StandardLib instance;
    return instance;
}

void StandardLib::discover() {
    path = Paths::executable().parent_path().parent_path() / "lib" / "stdlib";
}

} // namespace lang

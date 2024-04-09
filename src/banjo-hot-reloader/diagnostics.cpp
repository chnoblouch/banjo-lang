#include "diagnostics.hpp"

#include <cstdlib>
#include <iostream>

namespace Diagnostics {

void log(const std::string &message) {
    std::cout << "(hot reloader) " << message << std::endl;
}

void abort(const std::string &message) {
    std::cerr << "(hot reloader) error: " << message << std::endl;
    std::exit(1);
}

} // namespace Diagnostics

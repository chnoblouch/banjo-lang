#ifndef BANJO_CLI_MANIFEST_H
#define BANJO_CLI_MANIFEST_H

#include "target.hpp"

#include <string>
#include <utility>
#include <vector>

namespace banjo {
namespace cli {

struct Manifest {
    std::string name;
    std::string type;
    std::vector<std::string> packages;
    std::vector<std::string> libraries;
    std::vector<std::pair<Target, Manifest>> target_manifests;
};

} // namespace cli
} // namespace banjo

#endif

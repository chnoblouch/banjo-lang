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
    std::vector<std::string> args;
    std::vector<std::string> libraries;
    std::vector<std::string> macos_frameworks;
    std::vector<std::string> packages;
    std::vector<std::pair<Target, Manifest>> target_manifests;
    std::optional<std::string> build_script;
};

} // namespace cli
} // namespace banjo

#endif

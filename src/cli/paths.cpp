#include "paths.hpp"

#include "banjo/utils/paths.hpp"

namespace banjo {
namespace cli {
namespace paths {

std::filesystem::path installation_dir() {
    return Paths::executable().parent_path().parent_path();
}

std::filesystem::path toolchains_dir() {
    return installation_dir() / "toolchains";
}

} // namespace paths
} // namespace cli
} // namespace banjo

#ifndef BANJO_CLI_PATHS_H
#define BANJO_CLI_PATHS_H

#include <filesystem>

namespace banjo {
namespace cli {
namespace paths {

std::filesystem::path installation_dir();
std::filesystem::path toolchains_dir();

}
} // namespace cli
} // namespace banjo

#endif

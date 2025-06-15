#ifndef BANJO_CLI_TOOLCHAINS_H
#define BANJO_CLI_TOOLCHAINS_H

#include "banjo/utils/json.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace banjo {
namespace cli {

struct UnixToolchain {
    std::string linker_path;
    std::vector<std::string> linker_args;
    std::vector<std::string> extra_libs;
    std::vector<std::string> lib_dirs;
    std::string crt_dir;

    static UnixToolchain detect();

    JSONObject serialize();

private:
    void find_linker();
    void find_lib_dirs();
    void find_crt_dir();
    std::filesystem::path find_c_compiler();
};

} // namespace cli
} // namespace banjo

#endif

#ifndef BANJO_CLI_TARGET_H
#define BANJO_CLI_TARGET_H

#include <optional>
#include <string>
#include <vector>

namespace banjo {
namespace cli {

struct Target {
    std::string arch;
    std::string os;
    std::optional<std::string> env;

    Target();
    Target(std::string arch, std::string os);
    Target(std::string arch, std::string os, std::optional<std::string> env);

    std::string to_string() const;

    bool operator==(const Target &rhs) const = default;
    bool operator!=(const Target &rhs) const = default;

    static Target host();
    static std::vector<Target> list_available();
    static std::optional<std::string> get_default_env(const std::string &os);
};

} // namespace cli
} // namespace banjo

#endif

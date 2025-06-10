#ifndef BANJO_CLI_COMMON_H
#define BANJO_CLI_COMMON_H

#include <string_view>

namespace banjo {
namespace cli {

extern bool quiet;

void print_step(std::string_view message);
void print_clear_line();
void error(std::string_view message);

}
} // namespace banjo

#endif

#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include "config/config.hpp"

#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace lang {

struct ValueArgParser {
    std::string default_value;
};

struct ParsedArgs {
    std::unordered_map<std::string, std::string> values;
    std::unordered_map<std::string, bool> flags;
    std::unordered_map<std::string, std::vector<std::string>> lists;
};

class ArgumentParser {

private:
    std::unordered_map<std::string, ValueArgParser> values;
    std::unordered_set<std::string> flags;
    std::unordered_set<std::string> lists;

public:
    ArgumentParser &add_value(std::string name, std::string default_value);
    ArgumentParser &add_flag(std::string name);
    ArgumentParser &add_list(std::string name);
    ParsedArgs parse(int argc, char **argv);

private:
    ParsedArgs create_default_args();
};

} // namespace lang

#endif

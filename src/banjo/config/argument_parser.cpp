#include "argument_parser.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace banjo {

namespace lang {

ArgumentParser &ArgumentParser::add_value(std::string name, std::string default_value) {
    values.insert({name, {.default_value = default_value}});
    return *this;
}

ArgumentParser &ArgumentParser::add_flag(std::string name) {
    flags.insert(name);
    return *this;
}

ArgumentParser &ArgumentParser::add_list(std::string name) {
    lists.insert(name);
    return *this;
}

ParsedArgs ArgumentParser::parse(int argc, char **argv) {
    ParsedArgs result = create_default_args();

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (!arg.starts_with("--")) {
            continue;
        }

        std::string name = arg.substr(2);

        if (values.contains(name)) {
            result.values[name] = argv[++i];
        } else if (flags.contains(name)) {
            result.flags[name] = true;
        } else if (lists.contains(name)) {
            result.lists[name].push_back(argv[++i]);
        }
    }

    return result;
}

ParsedArgs ArgumentParser::create_default_args() {
    ParsedArgs args;

    for (const auto &value : values) {
        args.values[value.first] = value.second.default_value;
    }

    for (const std::string &flag : flags) {
        args.flags[flag] = false;
    }

    for (const std::string &list : lists) {
        args.lists[list] = {};
    }

    return args;
}

} // namespace lang

} // namespace banjo

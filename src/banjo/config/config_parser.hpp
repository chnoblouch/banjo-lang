#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "config/argument_parser.hpp"
#include "config/config.hpp"
#include "target/target_description.hpp"

namespace banjo {

namespace lang {

class ConfigParser {

private:
    ArgumentParser arg_parser;

public:
    ConfigParser();
    Config parse(int argc, char **argv);

private:
    Config create_config(const ParsedArgs &args);
    target::TargetDescription create_target(const ParsedArgs &args);
};

} // namespace lang

} // namespace banjo

#endif

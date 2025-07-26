#ifndef BANJO_CONFIG_PARSER_H
#define BANJO_CONFIG_PARSER_H

#include "banjo/config/argument_parser.hpp"
#include "banjo/config/config.hpp"
#include "banjo/target/target_description.hpp"

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

#ifndef BANJO_COMPILER_H
#define BANJO_COMPILER_H

#include "banjo/config/config.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/reports/report_printer.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/target/target.hpp"

namespace banjo {

namespace lang {

struct ModuleParseResult {
    bool exists;
};

class Compiler {

private:
    const Config &config;
    target::Target *target;
    ReportManager report_manager;
    ModuleManager module_manager;
    ReportPrinter report_printer;

public:
    Compiler(const Config &config);
    void compile();
};

} // namespace lang

} // namespace banjo

#endif

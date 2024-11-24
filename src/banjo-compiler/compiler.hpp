#ifndef COMPILER_H
#define COMPILER_H

#include "banjo/config/config.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/reports/report_printer.hpp"
#include "banjo/source/file_module_loader.hpp"
#include "banjo/source/module_manager.hpp"
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

    FileModuleLoader loader;
    ReportPrinter report_printer;

public:
    Compiler(const Config &config);
    void compile();

private:
    ssa::Module run_frontend();

    void run_middleend(ssa::Module &ir_module);
    void run_backend(ssa::Module &ir_module);
};

} // namespace lang

} // namespace banjo

#endif

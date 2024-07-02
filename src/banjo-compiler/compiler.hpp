#ifndef COMPILER_H
#define COMPILER_H

#include "config/config.hpp"
#include "ir/module.hpp"
#include "reports/report_manager.hpp"
#include "reports/report_printer.hpp"
#include "source/file_module_loader.hpp"
#include "source/module_manager.hpp"
#include "target/target.hpp"

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
    ir::Module run_frontend();

    void run_middleend(ir::Module &ir_module);
    void run_backend(ir::Module &ir_module);
};

} // namespace lang

} // namespace banjo

#endif

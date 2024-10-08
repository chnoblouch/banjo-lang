#ifndef JIT_COMPILER_H
#define JIT_COMPILER_H

#include "banjo/config/config.hpp"
#include "banjo/emit/binary_module.hpp"
#include "banjo/ir/addr_table.hpp"
#include "banjo/ir/module.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/source/file_module_loader.hpp"
#include "banjo/source/module_manager.hpp"

namespace banjo {

namespace hot_reloader {

class JITCompiler {

private:
    lang::Config &config;
    AddrTable &addr_table;
    target::Target *target;
    ir::Module ir_module;

    lang::ReportManager report_manager;
    lang::FileModuleLoader module_loader;
    lang::ModuleManager module_manager;

public:
    JITCompiler(lang::Config &config, AddrTable &addr_table);
    ~JITCompiler();

    bool build_ir();
    BinModule compile_func(const std::string &name);
    lang::ModuleManager &get_module_manager() { return module_manager; }
};

} // namespace hot_reloader

} // namespace banjo

#endif

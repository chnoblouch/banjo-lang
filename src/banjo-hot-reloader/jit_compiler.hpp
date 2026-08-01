#ifndef BANJO_HOT_RELOADER_JIT_COMPILER_H
#define BANJO_HOT_RELOADER_JIT_COMPILER_H

#include "banjo/config/config.hpp"
#include "banjo/emit/binary_module.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/ssa/addr_table.hpp"
#include "banjo/ssa/module.hpp"

#include <filesystem>

namespace banjo {

namespace hot_reloader {

class JITCompiler {

private:
    Config &config;
    ssa::AddrTable &addr_table;
    target::Target *target;
    ssa::Module ssa_module;

    ReportManager report_manager;
    ModuleManager module_manager;
    sir::Unit sir_unit;

public:
    JITCompiler(Config &config, ssa::AddrTable &addr_table);
    ~JITCompiler();

    bool build_ir();
    sir::Module *find_mod(const std::filesystem::path &absolute_path);
    BinModule compile_func(const std::string &name);
};

} // namespace hot_reloader

} // namespace banjo

#endif

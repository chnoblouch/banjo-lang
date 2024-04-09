#ifndef JIT_COMPILER_H
#define JIT_COMPILER_H

#include "config/config.hpp"
#include "emit/binary_module.hpp"
#include "ir/addr_table.hpp"
#include "reports/report_manager.hpp"
#include "source/file_module_loader.hpp"
#include "source/module_manager.hpp"
#include "symbol/data_type_manager.hpp"

class JITCompiler {

private:
    lang::Config &config;
    AddrTable &addr_table;
    target::Target *target_descr;
    ir::Module ir_module;
    lang::DataTypeManager type_manager;

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

#endif

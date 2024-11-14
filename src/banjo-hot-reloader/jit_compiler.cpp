#include "jit_compiler.hpp"

#include "banjo/codegen/ir_lowerer.hpp"
#include "banjo/codegen/machine_pass_runner.hpp"
#include "banjo/config/config.hpp"
#include "banjo/ir/addr_table.hpp"
#include "banjo/passes/addr_table_pass.hpp"
#include "banjo/reports/report_printer.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_generator.hpp"
#include "banjo/ssa_gen/ssa_generator.hpp"
#include "banjo/target/x86_64/x86_64_encoder.hpp"

namespace banjo {

namespace hot_reloader {

JITCompiler::JITCompiler(lang::Config &config, ir::AddrTable &addr_table)
  : config(config),
    addr_table(addr_table),
    target(target::Target::create(config.target, target::CodeModel::LARGE)),
    module_manager(module_loader, report_manager) {
    module_manager.add_standard_stdlib_search_path();
    module_manager.add_config_search_paths(config);
}

JITCompiler::~JITCompiler() {
    delete target;
}

bool JITCompiler::build_ir() {
    report_manager.reset();

    module_manager.clear();
    module_manager.load_all();

    sir_unit = lang::SIRGenerator().generate(module_manager.get_module_list());
    lang::sema::SemanticAnalyzer(sir_unit, target, report_manager).analyze();

    if (!report_manager.is_valid()) {
        lang::ReportPrinter report_printer(module_manager);
        if (config.color_diagnostics) {
            report_printer.enable_colors();
        }

        report_printer.print_reports(report_manager.get_reports());
        return false;
    }

    ssa_module = lang::SSAGenerator(sir_unit, target).generate();
    ssa_module.set_addr_table(addr_table);
    passes::AddrTablePass(target).run(ssa_module);

    return true;
}

lang::sir::Module *JITCompiler::find_mod(const std::filesystem::path &absolute_path) {
    for (lang::ASTModule *ast_mod : module_manager.get_module_list()) {
        if (std::filesystem::absolute(ast_mod->get_file_path()) == absolute_path) {
            return sir_unit.mods_by_path[ast_mod->get_path()];
        }
    }

    return nullptr;
}

BinModule JITCompiler::compile_func(const std::string &name) {
    ir::Module partial_ir_module;
    partial_ir_module.add(ssa_module.get_function(name));

    for (const ir::Global &global : ssa_module.get_globals()) {
        partial_ir_module.add(global);
    }

    for (const ir::FunctionDecl &external_func : ssa_module.get_external_functions()) {
        partial_ir_module.add(external_func);
    }

    for (const ir::GlobalDecl &external_global : ssa_module.get_external_globals()) {
        partial_ir_module.add(external_global);
    }

    for (ir::Structure *struct_ : ssa_module.get_structures()) {
        partial_ir_module.add(struct_);
    }

    codegen::IRLowerer *ir_lowerer = target->create_ir_lowerer();
    mcode::Module machine_module = ir_lowerer->lower_module(partial_ir_module);
    delete ir_lowerer;

    partial_ir_module.forget_pointers();

    codegen::MachinePassRunner(target).create_and_run(machine_module);
    return target::X8664Encoder().encode(machine_module);
}

} // namespace hot_reloader

} // namespace banjo

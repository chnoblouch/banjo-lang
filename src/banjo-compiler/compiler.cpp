#include "compiler.hpp"

#include "banjo/ast/ast_writer.hpp"
#include "banjo/codegen/machine_pass_runner.hpp"
#include "banjo/codegen/ssa_lowerer.hpp"
#include "banjo/config/config.hpp"
#include "banjo/passes/pass_runner.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_generator.hpp"
#include "banjo/sir/sir_printer.hpp"
#include "banjo/sir/test_driver_generator.hpp"
#include "banjo/ssa/module.hpp"
#include "banjo/ssa/writer.hpp"
#include "banjo/ssa_gen/ssa_generator.hpp"
#include "banjo/utils/timing.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace banjo {

namespace lang {

Compiler::Compiler(const Config &config)
  : config(config),
    module_manager(report_manager),
    report_printer(module_manager) {}

void Compiler::compile() {
    if (config.debug && !std::filesystem::is_directory("logs")) {
        std::filesystem::create_directories("logs");
    }

    target::CodeModel code_model = config.code_model ? *config.code_model : target::CodeModel::SMALL;
    target = target::Target::create(config.target, code_model);

    if (config.color_diagnostics) {
        report_printer.enable_colors();
    }

    ssa::Module ir_module = run_frontend();
    run_middleend(ir_module);
    run_backend(ir_module);

    PROFILE_SECTION_BEGIN("CLEANUP");

    delete target;
}

ssa::Module Compiler::run_frontend() {
    PROFILE_SECTION_BEGIN("FRONTEND");

    module_manager.add_standard_stdlib_search_path();
    module_manager.add_config_search_paths(config);
    module_manager.load_all();

    if (config.debug) {
        std::ofstream stream("logs/ast.bnj-tree");
        ASTWriter(stream).write_all(module_manager.get_module_list());
    }

    sir::Unit sir_unit = SIRGenerator().generate(module_manager.get_module_list());
    if (config.debug) {
        std::ofstream sir_file_generated("logs/sir.generated.txt");
        sir::Printer(sir_file_generated).print(sir_unit);
    }

    sema::SemanticAnalyzer(sir_unit, target, report_manager).analyze();
    if (config.debug) {
        std::ofstream sir_file_analyzed("logs/sir.analyzed.txt");
        sir::Printer(sir_file_analyzed).print(sir_unit);
    }

    if (config.testing) {
        sir_unit.mods.push_back(TestDriverGenerator(sir_unit, target, report_manager).generate());
    }

    report_printer.print_reports(report_manager.get_reports());
    if (!report_manager.is_valid()) {
        std::exit(EXIT_FAILURE);
    }

    PROFILE_SECTION_END("FRONTEND");
    PROFILE_SECTION_BEGIN("LOWERING");

    ssa::Module ssa_module = SSAGenerator(sir_unit, target).generate();

    if (config.debug) {
        std::ofstream stream("logs/ssa.input.cryoir");
        ssa::Writer(stream).write(ssa_module);
    }

    PROFILE_SECTION_END("LOWERING");

    return ssa_module;
}

void Compiler::run_middleend(ssa::Module &ir_module) {
    PROFILE_SECTION("OPTIMIZATION");

    passes::PassRunner pass_runner;
    pass_runner.set_opt_level(config.opt_level);
    pass_runner.set_generate_addr_table(config.hot_reload);
    pass_runner.set_debug(config.debug);
    pass_runner.run(ir_module, target);
}

void Compiler::run_backend(ssa::Module &ir_module) {
    PROFILE_SECTION("BACKEND");

    codegen::SSALowerer *ssa_lowerer = target->create_ssa_lowerer();
    mcode::Module machine_module = ssa_lowerer->lower_module(ir_module);
    delete ssa_lowerer;

    codegen::MachinePassRunner(target).create_and_run(machine_module);

    std::ofstream stream("main." + target->get_output_file_ext(), std::ios::binary);
    codegen::Emitter *emitter = target->create_emitter(machine_module, stream);
    emitter->generate();
    delete emitter;
}

} // namespace lang

} // namespace banjo

#include "compiler.hpp"

#include "banjo/ast/ast_writer.hpp"
#include "banjo/codegen/ir_lowerer.hpp"
#include "banjo/codegen/machine_pass_runner.hpp"
#include "banjo/config/config.hpp"
#include "banjo/ir/module.hpp"
#include "banjo/ir/writer.hpp"
#include "banjo/passes/pass_runner.hpp"
#include "banjo/utils/timing.hpp"

#if BANJO_ENABLE_SIR
#    include "banjo/sema2/semantic_analyzer.hpp"
#    include "banjo/sir/sir.hpp"
#    include "banjo/sir/sir_generator.hpp"
#    include "banjo/sir/sir_printer.hpp"
#    include "banjo/sir/test_driver_generator.hpp"
#    include "banjo/ssa_gen/ssa_generator.hpp"
#else
#    include "banjo/ast/test_module.hpp"
#    include "banjo/ir_builder/root_ir_builder.hpp"
#    include "banjo/sema/semantic_analyzer.hpp"
#    include "banjo/symbol/data_type_manager.hpp"
#endif

#include <filesystem>
#include <fstream>
#include <iostream>

namespace banjo {

namespace lang {

Compiler::Compiler(const Config &config)
  : config(config),
    module_manager(loader, report_manager),
    report_printer(module_manager) {}

void Compiler::compile() {
    if (config.is_debug() && !std::filesystem::is_directory("logs")) {
        std::filesystem::create_directories("logs");
    }

    target::CodeModel code_model = config.code_model ? *config.code_model : target::CodeModel::SMALL;
    target = target::Target::create(config.target, code_model);

    if (config.color_diagnostics) {
        report_printer.enable_colors();
    }

    ir::Module ir_module = run_frontend();
    run_middleend(ir_module);
    run_backend(ir_module);

    PROFILE_SECTION_BEGIN("CLEANUP");

    delete target;
}

ir::Module Compiler::run_frontend() {
    PROFILE_SECTION_BEGIN("FRONTEND");

    module_manager.add_standard_stdlib_search_path();
    module_manager.add_config_search_paths(config);
    module_manager.load_all();

    if (config.is_debug()) {
        std::ofstream stream("logs/ast.bnj-tree");
        ASTWriter(stream).write_all(module_manager.get_module_list());
    }

#if BANJO_ENABLE_SIR
    sir::Unit sir_unit = SIRGenerator().generate(module_manager.get_module_list());
    if (config.is_debug()) {
        std::ofstream sir_file_generated("logs/sir.generated.txt");
        sir::Printer(sir_file_generated).print(sir_unit);
    }

    sema::SemanticAnalyzer(sir_unit, target, report_manager).analyze();
    if (config.is_debug()) {
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
#else
    DataTypeManager type_manager;
    SemanticAnalyzer analyzer(module_manager, type_manager, target);

    if (config.testing) {
        analyzer.get_context().set_create_main_func(false);
    }

    SemanticAnalysis analysis = analyzer.analyze();
    report_manager.merge_result(std::move(analysis.reports), analysis.is_valid);
    report_printer.print_reports(report_manager.get_reports());

    if (config.is_debug()) {
        std::ofstream stream("logs/ast-analyzed.bnj-tree");
        ASTWriter(stream).write_all(module_manager.get_module_list());
    }

    if (!report_manager.is_valid()) {
        std::exit(EXIT_FAILURE);
    }

    if (config.testing) {
        ASTModule *test_module = TestModuleGenerator().generate(module_manager.get_module_list(), config);
        module_manager.get_module_list().add(test_module);
        SemanticAnalyzer(module_manager, type_manager, target).analyze_module(test_module);
    }
#endif

    PROFILE_SECTION_END("FRONTEND");
    PROFILE_SECTION_BEGIN("LOWERING");

#if BANJO_ENABLE_SIR
    ssa::Module ssa_module = SSAGenerator(sir_unit, target).generate();

    if (config.is_debug()) {
        std::ofstream stream("logs/ssa.input.cryoir");
        ir::Writer(stream).write(ssa_module);
    }
#else
    PROFILE_SCOPE_BEGIN("ir building");
    ir_builder::RootIRBuilder ir_builder(module_manager.get_module_list(), target);
    ssa::Module ssa_module = ir_builder.build();

    if (config.is_debug()) {
        std::ofstream stream("logs/ssa.input.cryoir");
        ir::Writer(stream).write(ssa_module);
    }

    PROFILE_SCOPE_END("ir building");
#endif

    PROFILE_SECTION_END("LOWERING");

    return ssa_module;
}

void Compiler::run_middleend(ir::Module &ir_module) {
    PROFILE_SECTION("OPTIMIZATION");

    passes::PassRunner pass_runner;
    pass_runner.set_opt_level(config.get_opt_level());
    pass_runner.set_generate_addr_table(config.is_hot_reload());
    pass_runner.set_debug(config.is_debug());
    pass_runner.run(ir_module, target);
}

void Compiler::run_backend(ir::Module &ir_module) {
    PROFILE_SECTION("BACKEND");

    codegen::IRLowerer *ir_lowerer = target->create_ir_lowerer();
    mcode::Module machine_module = ir_lowerer->lower_module(ir_module);
    delete ir_lowerer;

    codegen::MachinePassRunner(target).create_and_run(machine_module);

    std::ofstream stream("main." + target->get_output_file_ext(), std::ios::binary);
    codegen::Emitter *emitter = target->create_emitter(machine_module, stream);
    emitter->generate();
    delete emitter;
}

} // namespace lang

} // namespace banjo

#include "compiler.hpp"

#include "banjo/ast/ast_writer.hpp"
#include "banjo/ast/test_module.hpp"
#include "banjo/codegen/ir_lowerer.hpp"
#include "banjo/codegen/machine_pass_runner.hpp"
#include "banjo/config/config.hpp"
#include "banjo/ir/module.hpp"
#include "banjo/ir/validator.hpp"
#include "banjo/ir/writer.hpp"
#include "banjo/ir_builder/root_ir_builder.hpp"
#include "banjo/passes/pass_runner.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/utils/timing.hpp"

#include <cstdlib>
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

    PROFILE_SECTION_END("FRONTEND");
    PROFILE_SECTION_BEGIN("LOWERING");

    PROFILE_SCOPE_BEGIN("ir building");

    ir_builder::RootIRBuilder ir_builder(module_manager.get_module_list(), target);
    ir::Module ir_module = ir_builder.build();

    if (config.is_debug()) {
        std::ofstream stream("logs/ssa_input.cryoir");
        ir::Writer(stream).write(ir_module);
    }

    PROFILE_SCOPE_END("ir building");
    PROFILE_SECTION_END("LOWERING");

    return ir_module;
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

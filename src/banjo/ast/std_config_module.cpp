#include "std_config_module.hpp"

#include "banjo/ast/ast_block.hpp"
#include "banjo/ast/decl.hpp"
#include "banjo/ast/expr.hpp"
#include "banjo/config/config.hpp"
#include "banjo/symbol/symbol_table.hpp"

namespace banjo {

namespace lang {

enum ConfigArch { X86_64 = 0, AARCH64 = 1 };
enum ConfigOS { WINDOWS = 0, LINUX = 1, MACOS = 2, ANDROID = 3 };
enum ConfigBuildConfig { BUILD_CONFIG_DEBUG = 0, BUILD_CONFIG_RELEASE = 1 };

StdConfigModule::StdConfigModule() : ASTModule({"std", "config"}), config(Config::instance()) {
    block = new ASTBlock({0, 0}, nullptr);

    add_const_u32("DEBUG", BUILD_CONFIG_DEBUG);
    add_const_u32("RELEASE", BUILD_CONFIG_RELEASE);
    add_const_u32("BUILD_CONFIG", config.get_opt_level() == 0 ? BUILD_CONFIG_DEBUG : BUILD_CONFIG_RELEASE);

    add_const_u32("X86_64", X86_64);
    add_const_u32("AARCH64", AARCH64);
    add_const_u32("ARCH", get_arch());

    add_const_u32("WINDOWS", WINDOWS);
    add_const_u32("LINUX", LINUX);
    add_const_u32("MACOS", MACOS);
    add_const_u32("ANDROID", ANDROID);
    add_const_u32("OS", get_os());

    append_child(block);
}

unsigned StdConfigModule::get_arch() {
    switch (config.target.get_architecture()) {
        case target::Architecture::X86_64: return X86_64;
        case target::Architecture::AARCH64: return AARCH64;
        default: return 0;
    }
}

unsigned StdConfigModule::get_os() {
    switch (config.target.get_operating_system()) {
        case target::OperatingSystem::WINDOWS: return WINDOWS;
        case target::OperatingSystem::LINUX: return LINUX;
        case target::OperatingSystem::MACOS: return MACOS;
        case target::OperatingSystem::ANDROID: return ANDROID;
        default: return 0;
    }
}

void StdConfigModule::add_const_u32(const std::string &name, unsigned value) {
    ASTConst *config_const = new ASTConst();
    config_const->append_child(new Identifier(name, {0, 0}));
    config_const->append_child(new Expr(AST_U32));
    config_const->append_child(new IntLiteral(std::to_string(value)));
    block->append_child(config_const);
}

} // namespace lang

} // namespace banjo

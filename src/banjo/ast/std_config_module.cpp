#include "std_config_module.hpp"

#include "banjo/ast/ast_node.hpp"
#include "banjo/config/config.hpp"

#include <string_view>

namespace banjo {

namespace lang {

enum ConfigArch {
    X86_64 = 0,
    AARCH64 = 1,
    WASM = 2,
};

enum ConfigOS {
    WINDOWS = 0,
    LINUX = 1,
    MACOS = 2,
    ANDROID = 3,
    EMSCRIPTEN = 4,
    UNKNOWN = 5,
};

enum ConfigBuildConfig {
    BUILD_CONFIG_DEBUG = 0,
    BUILD_CONFIG_RELEASE = 1,
};

StdConfigModule::StdConfigModule(SourceFile &file) : ASTModule(file), config(Config::instance()) {
    block = create_node(AST_BLOCK, TextRange{0, 0});

    // add_const_enum("CUR_BUILD_CONFIG", "BuildConfig", {"DEBUG", "RELEASE"});
    // add_const_enum("CUR_ARCH", "Architecture", {"X86_64", "AARCH64", "WASM"});
    // add_const_enum("CUR_OS", "OperatingSystem", {"WINDOWS", "LINUX", "MACOS", "ANDROID", "EMSCRIPTEN", "UNKNOWN"});

    add_const_u32("DEBUG", BUILD_CONFIG_DEBUG);
    add_const_u32("RELEASE", BUILD_CONFIG_RELEASE);
    add_const_u32("BUILD_CONFIG", config.opt_level == 0 ? BUILD_CONFIG_DEBUG : BUILD_CONFIG_RELEASE);

    add_const_u32("X86_64", X86_64);
    add_const_u32("AARCH64", AARCH64);
    add_const_u32("WASM", WASM);
    add_const_u32("ARCH", get_arch());

    add_const_u32("WINDOWS", WINDOWS);
    add_const_u32("LINUX", LINUX);
    add_const_u32("MACOS", MACOS);
    add_const_u32("ANDROID", ANDROID);
    add_const_u32("EMSCRIPTEN", EMSCRIPTEN);
    add_const_u32("UNKNOWN", UNKNOWN);
    add_const_u32("OS", get_os());

    append_child(block);
}

unsigned StdConfigModule::get_arch() {
    switch (config.target.get_architecture()) {
        case target::Architecture::X86_64: return X86_64;
        case target::Architecture::AARCH64: return AARCH64;
        case target::Architecture::WASM: return WASM;
        default: return 0;
    }
}

unsigned StdConfigModule::get_os() {
    switch (config.target.get_operating_system()) {
        case target::OperatingSystem::WINDOWS: return WINDOWS;
        case target::OperatingSystem::LINUX: return LINUX;
        case target::OperatingSystem::MACOS: return MACOS;
        case target::OperatingSystem::ANDROID: return ANDROID;
        case target::OperatingSystem::EMSCRIPTEN: return EMSCRIPTEN;
        case target::OperatingSystem::UNKNOWN: return UNKNOWN;
        default: return 0;
    }
}

void StdConfigModule::add_const_u32(std::string_view name, unsigned value) {
    ASTNode *config_const = create_node(AST_CONSTANT);
    config_const->append_child(create_node(AST_IDENTIFIER, name, TextRange{0, 0}));
    config_const->append_child(create_node(AST_U32));
    config_const->append_child(create_node(AST_INT_LITERAL, string_arena.store(std::to_string(value))));
    block->append_child(config_const);
}

void StdConfigModule::add_const_enum(
    std::string_view const_name,
    std::string_view enum_name,
    std::initializer_list<std::string_view> values
) {
    ASTNode *enum_variants = create_node(AST_ENUM_VARIANT_LIST);

    for (std::string_view value : values) {
        ASTNode *enum_variant = create_node(AST_ENUM_VARIANT);
        enum_variant->append_child(create_node(AST_IDENTIFIER, value, TextRange{0, 0}));
        enum_variants->append_child(enum_variant);
    }

    ASTNode *enum_def = create_node(AST_ENUM_DEFINITION);
    enum_def->append_child(create_node(AST_IDENTIFIER, enum_name, TextRange{0, 0}));
    enum_def->append_child(enum_variants);
    block->append_child(enum_def);

    ASTNode *const_value_dot_expr = create_node(AST_DOT_OPERATOR);
    const_value_dot_expr->append_child(create_node(AST_IDENTIFIER, enum_name, TextRange{0, 0}));
    const_value_dot_expr->append_child(create_node(AST_IDENTIFIER, *values.begin(), TextRange{0, 0}));

    ASTNode *const_def = create_node(AST_CONSTANT);
    const_def->append_child(create_node(AST_IDENTIFIER, const_name, TextRange{0, 0}));
    const_def->append_child(create_node(AST_IDENTIFIER, enum_name, TextRange{0, 0}));
    const_def->append_child(const_value_dot_expr);
    block->append_child(const_def);
}

} // namespace lang

} // namespace banjo

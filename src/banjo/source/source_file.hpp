#ifndef BANJO_SOURCE_FILE_H
#define BANJO_SOURCE_FILE_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/module_path.hpp"

#include <filesystem>
#include <istream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace banjo::lang {

class SourceFile {

public:
    static constexpr char EOF_CHAR = '\0';
    static constexpr unsigned EOF_ZONE_SIZE = 2;

    ModulePath mod_path;
    std::vector<ModulePath> sub_mod_paths;
    std::filesystem::path fs_path;
    std::string buffer;
    std::vector<Token> tokens;
    std::unique_ptr<ASTModule> ast_mod;
    sir::Module *sir_mod;

    static std::unique_ptr<SourceFile> read(
        ModulePath mod_path,
        const std::filesystem::path &fs_path,
        std::istream &stream
    );

    void update_content(std::string content);
    std::string_view get_content() const;
};

} // namespace banjo::lang

#endif

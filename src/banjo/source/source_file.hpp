#ifndef BANJO_SOURCE_FILE_H
#define BANJO_SOURCE_FILE_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/lexer/token.hpp"
#include "banjo/sir/sir.hpp"

#include <filesystem>
#include <istream>
#include <string>
#include <string_view>
#include <vector>

namespace banjo::lang {

class SourceFile {

public:
    static constexpr char EOF_CHAR = '\0';
    static constexpr unsigned EOF_ZONE_SIZE = 2;

    std::filesystem::path fs_path;
    std::string buffer;
    std::vector<Token> tokens;
    ASTModule *ast_mod;
    sir::Module *sir_mod;

    static SourceFile read(std::filesystem::path fs_path, std::istream &stream);

    std::string_view get_buffer() { return buffer; }
    std::string_view get_content() { return std::string_view{buffer}.substr(0, buffer.size() - EOF_ZONE_SIZE); }
};

} // namespace banjo::lang

#endif

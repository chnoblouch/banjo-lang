add_executable(banjo-test-util
    "aarch64_asm_parser.cpp"
    "aarch64_asm_parser.hpp"    
    "assembler_lexer.cpp"
    "assembler_lexer.hpp"
    "assembly_util.cpp"
    "assembly_util.hpp"
    "formatter_util.cpp"
    "formatter_util.hpp"
    "main.cpp"
    "line_based_reader.cpp"
    "line_based_reader.hpp"
    "ssa_parser.cpp"
    "ssa_parser.hpp"
    "ssa_util.cpp"
    "ssa_util.hpp"
    "x86_64_asm_parser.cpp"
    "x86_64_asm_parser.hpp"
)

target_include_directories(banjo-test-util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(banjo-test-util PUBLIC banjo)

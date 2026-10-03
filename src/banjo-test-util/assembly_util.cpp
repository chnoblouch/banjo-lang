#include "assembly_util.hpp"

#include "assembler_lexer.hpp"
#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/target/aarch64/aarch64_encoder.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/target/x86_64/x86_64_encoder.hpp"
#include "banjo/utils/macros.hpp"

#include "aarch64_asm_parser.hpp"
#include "x86_64_asm_parser.hpp"

#include <string_view>
#include <utility>

namespace banjo::test {

using namespace assembler;

AssemblyUtil::AssemblyUtil(target::Architecture arch, std::string_view source) : arch{arch}, source{source} {}

WriteBuffer AssemblyUtil::assemble() {
    tokens = Lexer{source}.tokenize();

    mcode::Function *m_func = new mcode::Function{.name = "f"};
    mcode::BasicBlockIter m_block = m_func->basic_blocks.append({.label = "b"});

    while (tokens.get().type != TokenType::END_OF_FILE) {
        if (tokens.get().type == TokenType::END_OF_LINE) {
            tokens.advance();
            continue;
        }

        std::optional<mcode::Instruction> instr;

        switch (arch) {
            case target::Architecture::X86_64: instr = X8664AsmParser{tokens}.parse_instr(); break;
            case target::Architecture::AARCH64: instr = AArch64AsmParser{tokens}.parse_instr(); break;
            default: ASSERT_UNREACHABLE;
        }

        if (instr) {
            m_block->append(*instr);
        } else {
            break;
        }
    }

    mcode::Module m_mod;
    m_mod.add(m_func);

    target::TargetDescription target{
        arch,
        target::OperatingSystem::LINUX,
        target::Environment::GNU,
    };

    if (arch == target::Architecture::X86_64) {
        BinModule bin_mod = target::X8664Encoder{}.encode(m_mod);
        return std::move(bin_mod.text);
    } else if (arch == target::Architecture::AARCH64) {
        BinModule bin_mod = target::AArch64Encoder{target}.encode(m_mod);
        return std::move(bin_mod.text);
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace banjo::test

#include "assembly_util.hpp"

#include "banjo/emit/binary_module.hpp"
#include "banjo/mcode/function.hpp"
#include "banjo/mcode/instruction.hpp"
#include "banjo/mcode/module.hpp"
#include "banjo/mcode/parser.hpp"
#include "banjo/target/aarch64/aarch64_encoder.hpp"
#include "banjo/target/target.hpp"
#include "banjo/target/target_description.hpp"
#include "banjo/target/x86_64/x86_64_encoder.hpp"
#include "banjo/utils/generic_lexer.hpp"
#include "banjo/utils/macros.hpp"

#include <memory>
#include <string_view>
#include <utility>

namespace banjo::test {

AssemblyUtil::AssemblyUtil(target::Architecture arch) : arch{arch} {}

WriteBuffer AssemblyUtil::assemble(std::string_view source) {
    target::TargetDescription target_descr{arch, target::OperatingSystem::LINUX, target::Environment::GNU};
    target::Target *target = target::Target::create(target_descr, target::CodeModel::LARGE);

    utils::TokenStream tokens = utils::GenericLexer{source}.tokenize();
    std::unique_ptr<mcode::Parser> parser = target->create_parser(tokens);

    mcode::Function *m_func = new mcode::Function{.name = "f"};
    mcode::BasicBlockIter m_block = m_func->basic_blocks.append({.label = "b"});

    while (tokens.get().type != utils::TokenType::END_OF_FILE) {
        if (tokens.get().type == utils::TokenType::END_OF_LINE) {
            tokens.advance();
            continue;
        }

        if (std::optional<mcode::Instruction> instr = parser->parse_instr()) {
            m_block->append(*instr);
        } else {
            break;
        }
    }

    delete target;

    mcode::Module m_mod;
    m_mod.add(m_func);

    if (arch == target::Architecture::X86_64) {
        BinModule bin_mod = target::X8664Encoder{}.encode(m_mod);
        return std::move(bin_mod.text);
    } else if (arch == target::Architecture::AARCH64) {
        BinModule bin_mod = target::AArch64Encoder{target_descr}.encode(m_mod);
        return std::move(bin_mod.text);
    } else {
        ASSERT_UNREACHABLE;
    }
}

} // namespace banjo::test

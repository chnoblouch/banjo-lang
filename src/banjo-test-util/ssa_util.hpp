#ifndef BANJO_TEST_UTIL_SSA_UTIL_H
#define BANJO_TEST_UTIL_SSA_UTIL_H

#include "banjo/ssa/module.hpp"

#include <string_view>
#include <unordered_map>

namespace banjo::test {

class SSAUtil {

private:
    typedef std::unordered_map<ssa::VirtualRegister, ssa::VirtualRegister> RegMap;

public:
    void optimize(std::string_view pass_name);

private:
    void renumber_regs(ssa::Module &mod);
    void replace_regs(const RegMap &reg_map, ssa::Operand &operand);
};

} // namespace banjo::test

#endif

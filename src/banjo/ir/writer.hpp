#ifndef IR_WRITER_H
#define IR_WRITER_H

#include "ir/module.hpp"

#include <ostream>
#include <string>

namespace banjo {

namespace ir {

class Writer {

private:
    std::ostream &stream;

public:
    Writer(std::ostream &stream);
    void write(Module &mod);

private:
    void write_func_decl(FunctionDecl &func_decl);
    void write_func_def(Function *func_def);
    void write_basic_block(BasicBlock &basic_block);

    std::string reg_to_str(VirtualRegister reg);
    std::string type_to_str(Type type);
    std::string value_to_str(Value value);
    std::string calling_conv_to_str(CallingConv calling_conv);
};

} // namespace ir

} // namespace banjo

#endif

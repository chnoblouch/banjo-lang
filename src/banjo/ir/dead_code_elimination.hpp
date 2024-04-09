#ifndef IR_DEAD_CODE_ELIMINATION_H
#define IR_DEAD_CODE_ELIMINATION_H

#include "ir/module.hpp"

namespace ir {

namespace DeadCodeElimination {

void run(Module &mod);
void run(Function *func);

} // namespace DeadCodeElimination

} // namespace ir

#endif

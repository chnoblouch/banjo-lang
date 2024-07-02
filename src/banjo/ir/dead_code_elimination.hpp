#ifndef IR_DEAD_CODE_ELIMINATION_H
#define IR_DEAD_CODE_ELIMINATION_H

#include "ir/basic_block.hpp"
#include "ir/function.hpp"
#include "ir/operand.hpp"

#include <vector>

namespace banjo {

namespace ir {

class DeadCodeElimination {

private:
    struct ParamInfo {
        std::vector<ParamInfo *> direct_src_params;
        bool used;
    };

    struct BlockInfo {
        std::vector<ParamInfo> params;
        std::vector<BranchTarget *> preds;
    };

public:
    void run(Function &func);

private:
    void remove_unused_params(Function &func);
    void mark_as_used(ParamInfo *param_info);
    void remove_unused_instrs(Function &func);
};

} // namespace ir

} // namespace banjo

#endif

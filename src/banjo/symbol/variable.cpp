#include "variable.hpp"

namespace banjo {

namespace lang {

void DeinitInfo::set_unmanaged() {
    unmanaged = true;

    for (DeinitInfo &member : member_info) {
        member.set_unmanaged();
    }
}

Variable::Variable(ASTNode *node, DataType *data_type, std::string name) : Symbol(node, name), data_type(data_type) {}

Variable::Variable(ASTNode *node, std::string name) : Symbol(node, name), data_type(nullptr) {}

Variable::~Variable() {}

} // namespace lang

} // namespace banjo

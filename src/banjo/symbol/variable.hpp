#ifndef VARIABLE_H
#define VARIABLE_H

#include "banjo/ast/ast_node.hpp"
#include "banjo/ir_builder/storage.hpp"
#include "banjo/symbol/data_type.hpp"
#include "banjo/symbol/location.hpp"
#include "banjo/symbol/symbol.hpp"

#include <string>

namespace banjo {

namespace ir_builder {
class IRBuilderContext;
} // namespace ir_builder

namespace lang {

struct DeinitInfo {
    bool unmanaged = false;
    bool has_deinit = false;
    ir::VirtualRegister flag_reg = -1;
    std::vector<DeinitInfo> member_info;
    Location location;

    void set_unmanaged();
};

class Variable : public Symbol {

private:
    DataType *data_type;
    DeinitInfo deinit_info;

public:
    Variable(ASTNode *node, DataType *data_type, std::string name);
    Variable(ASTNode *node, std::string name);
    virtual ~Variable();

    ASTNode *get_node() { return node; }
    DataType *get_data_type() { return data_type; }
    DeinitInfo &get_deinit_info() { return deinit_info; }

    void set_data_type(DataType *data_type) { this->data_type = data_type; }

    virtual ir_builder::StoredValue as_ir_value(ir_builder::IRBuilderContext &context) = 0;
};

} // namespace lang

} // namespace banjo

#endif

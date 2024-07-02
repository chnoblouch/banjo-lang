#ifndef IR_FUNCTION_DECL_H
#define IR_FUNCTION_DECL_H

#include "ir/calling_conv.hpp"
#include "ir/type.hpp"

#include <string>
#include <utility>
#include <vector>

namespace banjo {

namespace ir {

class FunctionDecl {

private:
    std::string name;
    std::vector<Type> params;
    Type return_type;
    CallingConv calling_conv;
    bool global = false;

public:
    FunctionDecl() {}

    FunctionDecl(std::string name, std::vector<Type> params, Type return_type, CallingConv calling_conv)
      : name(std::move(name)),
        params(std::move(params)),
        return_type(std::move(return_type)),
        calling_conv(calling_conv) {}

    const std::string &get_name() const { return name; }
    const std::vector<Type> &get_params() const { return params; }
    Type get_return_type() const { return return_type; }
    CallingConv get_calling_conv() const { return calling_conv; }
    bool is_global() const { return global; }

    void set_name(std::string name) { this->name = std::move(name); }
    void set_params(std::vector<Type> params) { this->params = std::move(params); }
    void set_return_type(Type return_type) { this->return_type = std::move(return_type); }
    void set_calling_conv(CallingConv calling_conv) { this->calling_conv = calling_conv; }
    void set_global(bool global) { this->global = global; }
};

} // namespace ir

} // namespace banjo

#endif

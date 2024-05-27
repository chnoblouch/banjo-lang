#include "protocol.hpp"

namespace lang {

void Protocol::add_func_signature(std::string name, FunctionType type) {
    func_signatures.push_back({
        .name = std::move(name),
        .type = std::move(type),
        .index = static_cast<unsigned>(func_signatures.size()),
    });
}

FunctionSignature *Protocol::get_func_signature(const std::string &name) {
    for (FunctionSignature &func_signature : func_signatures) {
        if (func_signature.name == name) {
            return &func_signature;
        }
    }

    return nullptr;
}

} // namespace lang

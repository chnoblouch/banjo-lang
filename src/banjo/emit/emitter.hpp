#ifndef EMITTER_H
#define EMITTER_H

#include "mcode/module.hpp"
#include <ostream>

namespace banjo {

namespace codegen {

class Emitter {

protected:
    mcode::Module &module;
    std::ostream &stream;

public:
    Emitter(mcode::Module &module, std::ostream &stream) : module(module), stream(stream) {}
    virtual ~Emitter() = default;

    virtual void generate() = 0;
};

} // namespace codegen

} // namespace banjo

#endif

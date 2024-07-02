#ifndef PASSES_PASS_H
#define PASSES_PASS_H

#include "ir/module.hpp"
#include "target/target.hpp"

#include <ostream>
#include <string>

namespace banjo {

namespace passes {

class Pass {

private:
    std::string name;
    target::Target *target;
    std::ostream *logging_stream = nullptr;

public:
    Pass(std::string name, target::Target *target) : name(name), target(target) {}
    virtual ~Pass() = default;

    void enable_logging(std::ostream &stream) { logging_stream = &stream; }

    std::string get_name() { return name; }
    virtual void run(ir::Module &module) = 0;

protected:
    target::Target *get_target() { return target; }
    bool is_logging() { return logging_stream; }
    std::ostream &get_logging_stream() { return *logging_stream; }
};

} // namespace passes

} // namespace banjo

#endif

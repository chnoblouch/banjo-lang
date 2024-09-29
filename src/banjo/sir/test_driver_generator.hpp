#ifndef TEST_DRIVER_GENERATOR_H
#define TEST_DRIVER_GENERATOR_H

#include "banjo/reports/report_manager.hpp"
#include "banjo/sema2/decl_visitor.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/target/target.hpp"

#include <vector>

namespace banjo {

namespace lang {

class TestDriverGenerator {

private:
    class Visitor : sema::DeclVisitor {
    };

private:
    sir::Unit &unit;
    target::Target *target;
    ReportManager &report_manager;

public:
    TestDriverGenerator(sir::Unit &unit, target::Target *target, ReportManager &report_manager);
    sir::Module *generate();
};

} // namespace lang

} // namespace banjo

#endif

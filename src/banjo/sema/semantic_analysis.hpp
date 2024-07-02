#ifndef SEMANTIC_ANALYSIS_H
#define SEMANTIC_ANALYSIS_H

#include "banjo/reports/report.hpp"
#include <vector>

namespace banjo {

namespace lang {

struct SemanticAnalysis {
    bool is_valid;
    std::vector<Report> reports;
};

} // namespace lang

} // namespace banjo

#endif

#include "report_manager.hpp"

#include "reports/report_printer.hpp"

namespace banjo {

namespace lang {

void ReportManager::merge_result(std::vector<Report> reports, bool is_valid) {
    this->reports.insert(this->reports.end(), reports.begin(), reports.end());
    this->valid = this->valid && is_valid;
}

void ReportManager::reset() {
    reports.clear();
    valid = true;
}

} // namespace lang

} // namespace banjo

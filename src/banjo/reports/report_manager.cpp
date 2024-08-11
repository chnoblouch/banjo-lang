#include "report_manager.hpp"

#include <utility>

namespace banjo {

namespace lang {

Report &ReportManager::insert(Report report) {
    if (report.get_type() == Report::Type::ERROR) {
        valid = false;
    }

    reports.push_back(std::move(report));
    return reports.back();
}

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

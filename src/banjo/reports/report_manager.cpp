#include "report_manager.hpp"

#include <utility>

namespace banjo::lang {

Report &ReportManager::insert(Report report) {
    if (report.get_type() == Report::Type::ERROR) {
        valid = false;
    }

    reports.push_back(std::move(report));
    return reports.back();
}

void ReportManager::reset() {
    reports.clear();
    valid = true;
}

} // namespace banjo::lang

#ifndef BANJO_REPORTS_REPORT_MANAGER_H
#define BANJO_REPORTS_REPORT_MANAGER_H

#include "banjo/reports/report.hpp"

namespace banjo::lang {

class ReportManager {

private:
    std::vector<Report> reports;
    bool valid = true;

public:
    const std::vector<Report> &get_reports() const { return reports; }
    bool is_valid() const { return valid; }

    Report &insert(Report report);
    void reset();
};

} // namespace banjo::lang

#endif

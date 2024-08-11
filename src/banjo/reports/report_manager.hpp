#ifndef REPORT_MANAGER_H
#define REPORT_MANAGER_H

#include "banjo/reports/report.hpp"

namespace banjo {

namespace lang {

class ReportPrinter;

class ReportManager {

private:
    std::vector<Report> reports;
    bool valid = true;

public:
    const std::vector<Report> &get_reports() const { return reports; }
    bool is_valid() const { return valid; }

    Report &insert(Report report);
    void merge_result(std::vector<Report> reports, bool is_valid);
    void reset();
    void print_reports(ReportPrinter &printer);
};

} // namespace lang

} // namespace banjo

#endif

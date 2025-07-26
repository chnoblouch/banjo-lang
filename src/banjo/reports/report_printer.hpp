#ifndef BANJO_REPORTS_REPORT_PRINTER_H
#define BANJO_REPORTS_REPORT_PRINTER_H

#include "banjo/reports/report.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/source/text_range.hpp"

#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace banjo {

namespace lang {

class ReportPrinter {

private:
    struct LinePosition {
        unsigned number;
        unsigned offset;
    };

    std::ostream &stream;
    ModuleManager &module_manager;
    bool color_enabled = false;

public:
    ReportPrinter(ModuleManager &module_manager);
    void enable_colors() { color_enabled = true; }
    void print_reports(const std::vector<Report> &reports);
    void print_report(const Report &report);

private:
    void print_message_location(const SourceLocation &location);

    LinePosition find_line(std::ifstream &file, TextPosition position);
    unsigned find_column(std::ifstream &file, const ReportMessage &message, LinePosition line_position);

    void print_decorated_line(std::ifstream &file, TextRange range, std::string_view prefix);
    void print_decorated_line(std::ifstream &file, std::string_view prefix);

    void print_char(int c);
    unsigned char_width(int c);
    void set_color(std::string_view color);
};

} // namespace lang

} // namespace banjo

#endif

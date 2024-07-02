#ifndef REPORT_PRINTER_H
#define REPORT_PRINTER_H

#include "reports/report.hpp"
#include "source/module_manager.hpp"
#include "source/text_range.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace banjo {

namespace lang {

class ReportPrinter {

private:
    struct LinePosition {
        unsigned int number;
        unsigned int offset;
    };

    struct PrintableLine {
        std::string line;
        unsigned int underline_start;
        unsigned int underline_length;
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
    LinePosition find_line(const SourceLocation &location, std::ifstream &file);
    unsigned int find_column(const ReportMessage &message, std::ifstream &file, LinePosition line_position);
    PrintableLine get_printable_line(const SourceLocation &location, std::ifstream &file);
    void set_color(std::string color);
};

} // namespace lang

} // namespace banjo

#endif

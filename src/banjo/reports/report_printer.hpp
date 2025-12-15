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
    struct VisualPosition {
        unsigned offset;
        unsigned line_offset;
        unsigned line_end_offset;
        unsigned line_number;
        unsigned column_number;
    };

    struct VisualRange {
        VisualPosition start;
        VisualPosition end;
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

    VisualPosition find_position(const SourceFile &file, TextPosition position);
    VisualPosition find_visible_line_start(const SourceFile &file, VisualPosition position);
    VisualPosition find_visible_line_end(const SourceFile &file, VisualPosition position);
    bool is_whitespace(char c);

    void print_decorated_line(const SourceFile &file, VisualRange range, std::string_view prefix);

    void print_char(int c);
    unsigned char_width(int c);
    void set_color(std::string_view color);

};

} // namespace lang

} // namespace banjo

#endif

#include "report_printer.hpp"

#include "banjo/utils/utf8_encoding.hpp"

namespace banjo {

namespace lang {

constexpr unsigned MAX_PRINTED_REPORTS = 256;

ReportPrinter::ReportPrinter(ModuleManager &module_manager) : stream(std::cerr), module_manager(module_manager) {}

void ReportPrinter::print_reports(const std::vector<Report> &reports) {
    unsigned int num_errors = 0;
    unsigned int num_warnings = 0;

    unsigned num_reports_printed = 0;

    for (const Report &report : reports) {
        switch (report.get_type()) {
            case Report::Type::ERROR: num_errors++; break;
            case Report::Type::WARNING: num_warnings++; break;
        }

        if (num_reports_printed < MAX_PRINTED_REPORTS) {
            print_report(report);
        }

        num_reports_printed += 1;
        if (num_reports_printed == MAX_PRINTED_REPORTS) {
            std::cerr << num_reports_printed << " reports printed, stopping now.\n";
        }
    }

    if (num_errors > 1) {
        std::cerr << num_errors << " errors generated." << std::endl;
    } else if (num_errors == 1) {
        std::cerr << "1 error generated." << std::endl;
    }

    if (num_warnings > 1) {
        std::cerr << num_warnings << " warnings generated." << std::endl;
    } else if (num_warnings == 1) {
        std::cerr << "1 warning generated." << std::endl;
    }
}

void ReportPrinter::print_report(const Report &report) {
    switch (report.get_type()) {
        case Report::Type::ERROR:
            set_color("[31;22m");
            std::cerr << "error";
            break;
        case Report::Type::WARNING:
            set_color("[35;22m");
            std::cerr << "warning";
            break;
    }

    set_color("[97;22m");
    std::cerr << ": " << report.get_message().text << "\n";
    print_message_location(*report.get_message().location);
    std::cerr << "\n";

    for (const ReportMessage &note : report.get_notes()) {
        set_color("[36;22m");
        std::cerr << "note";
        set_color("[97;22m");
        std::cerr << ": " << note.text << "\n";

        if (note.location) {
            print_message_location(*note.location);
        }

        std::cerr << "\n";
    }

    set_color("[0m");
}

void ReportPrinter::print_message_location(const SourceLocation &location) {
    std::filesystem::path file_path = *module_manager.find_source_file(location.path);
    file_path.make_preferred();
    std::ifstream file(file_path, std::ios::binary);

    LinePosition line_position = find_line(location, file);
    unsigned int line_number = line_position.number;
    unsigned int column_number = location.range.start - line_position.offset + 1;

    PrintableLine printable_line = get_printable_line(location, file);
    std::string line_prefix = std::to_string(line_position.number) + " | ";

    std::cerr << "at " << file_path.string() << ":" << line_number << ":" << column_number << "\n";
    std::cerr << line_prefix << printable_line.line << '\n';
    std::cerr << std::string(line_prefix.length() + printable_line.underline_start, ' ');
    std::cerr << std::string(printable_line.underline_length, '~');
    std::cerr << "\n";
}

ReportPrinter::LinePosition ReportPrinter::find_line(const SourceLocation &location, std::ifstream &file) {
    file.seekg(0, std::ios::beg);

    LinePosition position{.number = 1, .offset = 0};

    for (int c = file.get(); c != EOF; c = file.get()) {
        if (c == '\n') {
            position.number++;
            position.offset = file.tellg();
        }

        if (file.tellg() == location.range.start) {
            file.seekg(position.offset, std::ios::beg);
            return position;
        }
    }

    return position;
}

ReportPrinter::PrintableLine ReportPrinter::get_printable_line(const SourceLocation &location, std::ifstream &file) {
    PrintableLine printable_line{.line = "", .underline_start = 0, .underline_length = 0};

    int printed_offset = 0;

    for (char c = (char)file.get(); c != EOF && c != '\n'; c = (char)file.get()) {
        if (c == '\t') {
            printable_line.line += "    ";
            printed_offset += 4;
        } else {
            printable_line.line += c;
            if (UTF8Encoding::is_first_byte_of_char(c)) {
                printed_offset += 1;
            }
        }

        if (file.tellg() == location.range.start) {
            printable_line.underline_start = printed_offset;
        }

        if (file.tellg() == location.range.end) {
            printable_line.underline_length = printed_offset - printable_line.underline_start;
        }
    }

    return printable_line;
}

void ReportPrinter::set_color(std::string color) {
    if (color_enabled) {
        stream << '\u001b' << color;
    }
}

} // namespace lang

} // namespace banjo

#include "report_printer.hpp"

#include "banjo/utils/utf8_encoding.hpp"

namespace banjo {

namespace lang {

constexpr unsigned MAX_PRINTED_REPORTS = 256;

ReportPrinter::ReportPrinter(ModuleManager &module_manager) : stream(std::cerr), module_manager(module_manager) {}

void ReportPrinter::print_reports(const std::vector<Report> &reports) {
    unsigned num_errors = 0;
    unsigned num_warnings = 0;

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
        std::cerr << num_errors << " errors generated.\n";
    } else if (num_errors == 1) {
        std::cerr << "1 error generated.\n";
    }

    if (num_warnings > 1) {
        std::cerr << num_warnings << " warnings generated.\n";
    } else if (num_warnings == 1) {
        std::cerr << "1 warning generated.\n";
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
    if (location.path.is_empty()) {
        std::cerr << "(source location unknown)\n";
        return;
    }

    std::filesystem::path file_path = *module_manager.find_source_file(location.path);
    file_path.make_preferred();
    std::ifstream file(file_path, std::ios::binary);

    LinePosition start_position = find_line(file, location.range.start);
    LinePosition end_position = find_line(file, location.range.end);

    unsigned line_number = start_position.number;
    unsigned column_number = location.range.start - start_position.offset + 1;
    std::cerr << "at " << file_path.string() << ":" << line_number << ":" << column_number << "\n";

    if (start_position.number == end_position.number) {
        std::string prefix = std::to_string(start_position.number);
        file.seekg(start_position.offset, std::ios::beg);
        print_decorated_line(file, location.range, prefix);
    } else {
        std::string start_prefix = std::to_string(start_position.number);
        std::string end_prefix = std::to_string(end_position.number);

        if (end_position.number > start_position.number) {
            start_prefix = std::string(end_prefix.size() - start_prefix.size(), ' ') + start_prefix;
        }

        file.seekg(start_position.offset, std::ios::beg);
        print_decorated_line(file, start_prefix);
        std::cerr << "...\n\n";
        file.seekg(end_position.offset, std::ios::beg);
        print_decorated_line(file, end_prefix);
    }
}

ReportPrinter::LinePosition ReportPrinter::find_line(std::ifstream &file, TextPosition position) {
    file.seekg(0, std::ios::beg);

    LinePosition line_position{.number = 1, .offset = 0};

    for (int c = file.get(); c != EOF; c = file.get()) {
        if (c == '\n') {
            line_position.number++;
            line_position.offset = file.tellg();
        }

        if (file.tellg() == position) {
            return line_position;
        }
    }

    return line_position;
}

void ReportPrinter::print_decorated_line(std::ifstream &file, TextRange range, std::string_view prefix) {
    unsigned printed_offset = 0;
    unsigned underline_start = 0;
    unsigned underline_length = 0;

    std::cerr << prefix << " | ";

    for (int c = file.get(); c != EOF && c != '\n'; c = file.get()) {
        print_char(c);
        printed_offset += char_width(c);

        if (file.tellg() == range.start) {
            underline_start = printed_offset;
        }

        if (file.tellg() == range.end) {
            underline_length = printed_offset - underline_start;
        }
    }

    std::cerr << '\n';
    std::cerr << std::string(prefix.length() + 3 + underline_start, ' ');
    std::cerr << std::string(underline_length, '~');
    std::cerr << '\n';
}

void ReportPrinter::print_decorated_line(std::ifstream &file, std::string_view prefix) {
    unsigned printed_offset = 0;
    std::optional<unsigned> underline_start;
    unsigned underline_length = 0;

    std::cerr << prefix << " | ";

    for (int c = file.get(); c != EOF && c != '\n'; c = file.get()) {
        unsigned width = char_width(c);
        print_char(c);

        if (c != ' ' && c != '\t') {
            if (underline_start) {
                underline_length = printed_offset - *underline_start;
            } else {
                underline_start = printed_offset;
            }
        }

        printed_offset += width;
    }

    std::cerr << '\n';

    if (underline_start) {
        std::cerr << std::string(prefix.length() + 3 + *underline_start, ' ');
        std::cerr << std::string(underline_length, '~');
    }

    std::cerr << '\n';
}

void ReportPrinter::print_char(int c) {
    if (c == '\t') {
        std::cerr << "    ";
    } else {
        std::cerr << static_cast<char>(c);
    }
}

unsigned ReportPrinter::char_width(int c) {
    if (c == '\t') {
        return 4;
    } else {
        return UTF8Encoding::is_first_byte_of_char(c) ? 1 : 0;
    }
}

void ReportPrinter::set_color(std::string_view color) {
    if (color_enabled) {
        stream << '\u001b' << color;
    }
}

} // namespace lang

} // namespace banjo

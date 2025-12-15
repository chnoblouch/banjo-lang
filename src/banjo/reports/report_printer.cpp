#include "report_printer.hpp"

#include "banjo/reports/report.hpp"
#include "banjo/source/source_file.hpp"
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

    const SourceFile &file = *module_manager.get_module_list().find(location.path);
    VisualPosition start_position = find_position(file, location.range.start);
    VisualPosition end_position = find_position(file, location.range.end);

    std::filesystem::path fs_path{file.fs_path};
    fs_path.make_preferred();

    unsigned line_number = start_position.line_number;
    unsigned column_number = start_position.column_number;
    std::cerr << "at " << fs_path.string() << ":" << line_number << ":" << column_number << "\n";

    if (start_position.line_number == end_position.line_number) {
        std::string prefix = std::to_string(start_position.line_number);
        print_decorated_line(file, {start_position, end_position}, prefix);
    } else {
        std::string start_prefix = std::to_string(start_position.line_number);
        std::string end_prefix = std::to_string(end_position.line_number);

        if (end_position.line_number > start_position.line_number) {
            start_prefix = std::string(end_prefix.size() - start_prefix.size(), ' ') + start_prefix;
        }

        VisualRange start_range{
            start_position,
            find_visible_line_end(file, start_position),
        };

        VisualRange end_range{
            find_visible_line_start(file, end_position),
            end_position,
        };

        print_decorated_line(file, start_range, start_prefix);
        std::cerr << "...\n\n";
        print_decorated_line(file, end_range, end_prefix);
    }
}

ReportPrinter::VisualPosition ReportPrinter::find_position(const SourceFile &file, TextPosition position) {
    unsigned line_offset = 0;
    unsigned line_number = 1;

    for (TextPosition i = 0; i < position; i++) {
        if (file.buffer[i] == '\n') {
            line_offset = i + 1;
            line_number += 1;
        }
    }

    unsigned line_end_offset = line_offset;

    for (unsigned i = line_offset; i < file.buffer.size(); i++) {
        char c = file.buffer[i];

        if (c == '\n' || c == SourceFile::EOF_CHAR) {
            line_end_offset = i;
            break;
        }
    }

    return VisualPosition{
        .offset = position,
        .line_offset = line_offset,
        .line_end_offset = line_end_offset,
        .line_number = line_number,
        .column_number = position - line_offset + 1,
    };
}

ReportPrinter::VisualPosition ReportPrinter::find_visible_line_start(const SourceFile &file, VisualPosition position) {
    VisualPosition result = position;

    for (unsigned i = position.line_offset; i <= position.offset; i++) {
        if (!is_whitespace(file.buffer[i])) {
            result.offset = i;
            break;
        }
    }

    return result;
}

ReportPrinter::VisualPosition ReportPrinter::find_visible_line_end(const SourceFile &file, VisualPosition position) {
    VisualPosition result = position;

    for (unsigned i = position.offset; i < position.line_end_offset; i++) {
        if (!is_whitespace(file.buffer[i])) {
            result.offset = i + 1;
        }
    }

    return result;
}

bool ReportPrinter::is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\t';
}

void ReportPrinter::print_decorated_line(const SourceFile &file, VisualRange range, std::string_view prefix) {
    unsigned printed_offset = 0;
    unsigned underline_start = 0;
    unsigned underline_length = 0;

    std::cerr << prefix << " | ";

    for (unsigned i = range.start.line_offset; i <= range.start.line_end_offset; i++) {
        char c = file.buffer[i];

        if (i == range.start.offset) {
            underline_start = printed_offset;
        }

        if (i == range.end.offset) {
            underline_length = printed_offset - underline_start;
        }

        if (i != range.start.line_end_offset) {
            print_char(c);
            printed_offset += char_width(c);
        }
    }

    std::cerr << '\n';
    std::cerr << std::string(prefix.length() + 3 + underline_start, ' ');
    std::cerr << std::string(underline_length, '~');
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

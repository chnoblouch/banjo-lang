#ifndef BANJO_REPORTS_REPORT_H
#define BANJO_REPORTS_REPORT_H

#include "banjo/reports/report_texts.hpp"
#include "banjo/source/text_range.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace banjo {

namespace lang {

class SourceFile;

struct SourceLocation {
    SourceFile *file;
    TextRange range;
};

struct ReportMessage {
    std::string text;
    std::optional<SourceLocation> location;

    ReportMessage() {}
    ReportMessage(std::string text) : text(text) {}
    ReportMessage(std::string text, SourceLocation location) : text(text), location(location) {}

    template <typename... FormatArgs>
    ReportMessage(SourceLocation location, std::string_view format_str, FormatArgs... format_args)
      : location(location) {
        ReportText message(format_str);
        ([&]() { message = message.format(format_args); }(), ...);
        text = message.str();
    }
};

class Report {

public:
    enum class Type { ERROR, WARNING };

private:
    Type type;
    ReportMessage message;
    std::vector<ReportMessage> notes;

public:
    Report(Type type);
    Report(Type type, ReportMessage message);
    Report(Type type, std::string message, TextRange range);
    Report(Type type, std::string message);
    Report(Type type, SourceLocation location);

    Type get_type() const { return type; }
    const ReportMessage &get_message() const { return message; }
    const std::vector<ReportMessage> &get_notes() const { return notes; }

    Report &set_message(std::string message);
    Report &set_message(ReportMessage message);
    Report &add_note(ReportMessage note);
};

} // namespace lang

} // namespace banjo

#endif

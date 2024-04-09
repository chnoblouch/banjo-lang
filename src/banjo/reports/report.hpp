#ifndef REPORT_H
#define REPORT_H

#include "reports/report_texts.hpp"
#include "source/text_range.hpp"
#include "symbol/module_path.hpp"

#include <string>
#include <vector>

namespace lang {

struct SourceLocation {
    ModulePath path;
    TextRange range;
};

struct ReportMessage {
    std::string text;
    std::optional<SourceLocation> location;

    ReportMessage(std::string text) : text(text) {}
    ReportMessage(std::string text, SourceLocation location) : text(text), location(location) {}

    template <typename... FormatArgs>
    ReportMessage(ReportText::ID text_id, FormatArgs... format_args) {
        ReportText message(text_id);
        ([&]() { message = message.format(format_args); }(), ...);
        text = message.str();
    }

    template <typename... FormatArgs>
    ReportMessage(SourceLocation location, ReportText::ID text_id, FormatArgs... format_args) : location(location) {
        ReportText message(text_id);
        ([&]() { message = message.format(format_args); }(), ...);
        text = message.str();
    }

    template <typename... FormatArgs>
    ReportMessage(TextRange range, ReportText::ID text_id, FormatArgs... format_args)
      : ReportMessage({{}, range}, text_id, format_args...) {}
};

class Report {

public:
    enum class Type { ERROR, WARNING };

private:
    Type type;
    ReportMessage message;
    std::vector<ReportMessage> notes;

public:
    Report(Type type, ReportMessage message);
    Report(Type type, std::string message, TextRange range);
    Report(Type type, std::string message);
    Report(Type type, SourceLocation location);

    Type get_type() const { return type; }
    const ReportMessage &get_message() const { return message; }
    const std::vector<ReportMessage> &get_notes() const { return notes; }

    Report &set_message(std::string message);
    Report &set_message(ReportText::ID text_id);
    Report &add_note(ReportMessage note);
};

} // namespace lang

#endif

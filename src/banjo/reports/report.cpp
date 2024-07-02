#include "report.hpp"

#include <utility>

namespace banjo {

namespace lang {

Report::Report(Type type, ReportMessage message) : type(type), message(std::move(message)) {}

Report::Report(Type type, std::string message, TextRange range)
  : type(type),
    message{std::move(message), SourceLocation{{}, range}} {}

Report::Report(Type type, std::string message) : type(type), message{std::move(message), SourceLocation{{}, {0, 0}}} {}

Report::Report(Type type, SourceLocation location) : type(type), message{"", std::move(location)} {}

Report &Report::set_message(std::string message) {
    this->message.text = std::move(message);
    return *this;
}

Report &Report::set_message(ReportText::ID text_id) {
    return set_message(ReportText(text_id).str());
}

Report &Report::add_note(ReportMessage note) {
    notes.push_back(std::move(note));
    return *this;
}

} // namespace lang

} // namespace banjo

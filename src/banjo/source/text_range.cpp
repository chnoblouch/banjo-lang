#include "text_range.hpp"

#include "banjo/lexer/token.hpp"

namespace banjo {

namespace lang {

TextRange::TextRange(TextPosition start, TextPosition end) : start(start), end(end) {}

TextRange::TextRange(Token &start, Token &end) : start(start.position), end(end.end()) {}

} // namespace lang

} // namespace banjo

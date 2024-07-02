#include "text_range.hpp"

#include "lexer/token.hpp"

namespace banjo {

namespace lang {

TextRange::TextRange(TextPosition start, TextPosition end) : start(start), end(end) {}

TextRange::TextRange(Token &start, Token &end) : start(start.get_position()), end(end.get_end()) {}

} // namespace lang

} // namespace banjo

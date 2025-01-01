#ifndef TEXT_RANGE_H
#define TEXT_RANGE_H

namespace banjo {

namespace lang {

struct Token;

typedef unsigned TextPosition;

struct TextRange {
    TextPosition start;
    TextPosition end;

    TextRange(TextPosition start, TextPosition end);
    TextRange(Token &start, Token &end);

    friend bool operator==(const TextRange &left, const TextRange &right) {
        return left.start == right.start && left.end == right.end;
    }
};

} // namespace lang

} // namespace banjo

#endif

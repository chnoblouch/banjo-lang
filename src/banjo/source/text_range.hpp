#ifndef TEXR_RANGE_H
#define TEXR_RANGE_H

namespace lang {

class Token;
class ASTNode;

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

#endif

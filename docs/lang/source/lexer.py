from pygments.lexer import RegexLexer
from pygments import token

KEYWORDS = [
    "var",
    "const",
    "func",
    "as",
    "if",
    "else",
    "switch",
    "case",
    "while",
    "for",
    "in",
    "break",
    "continue",
    "return",
    "struct",
    "enum",
    "union",
    "false",
    "true",
    "null",
    "none",
    "undefined",
    "use",
    "pub",
    "native",
    "meta",
    "type"
]

class BanjoLexer(RegexLexer):
    name = "banjo"

    tokens = {
        "root": [
            (r"\s+", token.Whitespace),
            (rf"\b({'|'.join(KEYWORDS)})\b", token.Keyword),
            (r"\bi8|i16|i32|i64|u8|u16|u32|u64|f32|f64|usize|bool|addr|void\b", token.Keyword.Type),
            (r"[+\-*/%&|!\^~=><.;(){}\[\]:,?]", token.Punctuation),
            (r"[a-zA-Z0-9_]", token.Name),
            (r"(0x|0b)?[0-9]+(\.[0-9])*", token.Number.Integer),
            (r"\bself\b", token.Name.Builtin),
            (r"\"", token.String, "string"),
            (r"#.+", token.Comment.SingleLine),
            (r"@(\[[a-zA-Z0-9_\=]+\]|[a-zA-Z0-9_\=]+)", token.Name.Attribute)
        ],
        "string": [
            (r"\"", token.String, "root"),
            (r"[^\"]+", token.String)
        ]
    }

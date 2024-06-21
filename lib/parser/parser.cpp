#include <cctype>
#include <string>
#include <string_view>
#include <vector>
#include <format>

struct Token {
    enum Kind {
        UNKNOWN,
        EOF_,
        LIST,
    } kind;

    std::string identifier;
    std::vector<Token> elements;
    ssize_t number;

    static void Print(const Token& token) {
        switch (token.kind) {
        case UNKNOWN:
            printf("Unknown");
            break;
        case EOF_:
            printf("EOF");
            break;
        case LIST: {
            printf("(");
            for (const auto& elem : token.elements) {
                Token::Print(elem);
                printf(", ");
            }
            printf(")");
        } break;
        }
    }
};

#define LEX_LIST_BEGIN '('
#define LEX_LIST_END   ')'

bool isdelimiter(const char c) {
    return c == LEX_LIST_BEGIN
           or c == LEX_LIST_END;
}

auto lex(std::string_view& source) -> Token {
    static const Token eof{Token::Kind::EOF_};
    Token out{};

    // Eat whitespace.
    while (source.size() and isspace(source.data()[0]))
        source.remove_prefix(1);

    // Cannot lex a token from an empty source.
    if (source.empty()) return eof;

    // Get first character from source.
    const auto c = source.data()[0];
    // Eat it.
    source.remove_prefix(1);

    // Identifier
    if (isalpha(c)) {
        // TODO: lex identifier
        out.identifier = std::string{c};
        // Until character is whitespace or delimiter...
        while (not isspace(source.data()[0]) and not isdelimiter(source.data()[0])) {
            // Add character to identifier.
            out.identifier += tolower(source.data()[0]);
            // Now eat it.
            source.remove_prefix(1);
        }
        printf("Lexed identifier %s\n", out.identifier.data());
        return out;
    }
    // List
    if (c == LEX_LIST_BEGIN) {
        // Parse list contents (fancy fun LISP tunnels)
        while (source.size() and source.data()[0] != LEX_LIST_END) {
            // TODO: parse() and stuff
        }
        source.remove_prefix(1);
    }
    // Number
    if (isdigit(c)) {
        // TODO: lex number
    }

    // TODO: unreachable
    return out;
}

void parse(std::string_view source) {
    while (source.size()) {
        auto token = lex(source);
        if (token.kind == Token::Kind::EOF_)
            break;
        if (token.kind != Token::Kind::LIST) {
            printf("ERROR: Unexpected token at top level; this is LISP, so use lists!\n");
            return;
        }

        // First element of list should be an identifier that will help us to
        // parse this meaningfully into the build scenario.

        // TARGET CREATION
        // "executable", "library", "target"

        // TARGET RELATED
        // "sources", "include-directories"

        // TARGET REQUISITE REGISTRATION
        // "command", "copy", "dependency"
    }
}

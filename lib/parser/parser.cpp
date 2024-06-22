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
        IDENTIFIER,
    } kind;

    std::string identifier;
    std::vector<Token> elements;

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
            bool notfirst = false;
            for (const auto& elem : token.elements) {
                if (notfirst) printf(", ");
                Token::Print(elem);
                notfirst = true;
            }
            printf(")");
        } break;
        case IDENTIFIER:
            printf("ID:\"%s\"", token.identifier.data());
            break;
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
        out.kind = Token::Kind::IDENTIFIER;
        // TODO: lex identifier
        out.identifier = std::string{c};
        // Until character is whitespace or delimiter...
        while (not isspace(source.data()[0]) and not isdelimiter(source.data()[0])) {
            // Add character to identifier.
            out.identifier += tolower(source.data()[0]);
            // Now eat it.
            source.remove_prefix(1);
        }
        return out;
    }
    // List
    if (c == LEX_LIST_BEGIN) {
        out.kind = Token::Kind::LIST;
        // Parse list contents (fancy fun LISP tunnels)
        while (source.size() and source.data()[0] != LEX_LIST_END) {
            out.elements.push_back(lex(source));
        }
        // Eat list closing character.
        return out;
        source.remove_prefix(1);
    }
    // Number
    if (isdigit(c)) {
        // TODO: lex number OR unreachable?
    }

    // TODO: unreachable
    return out;
}

void parse(std::string_view source) {
    // TODO: The idea is this will parse the source into a list of actions to
    // perform (i.e. shell commands to run for targets and that sort of thing).
    while (source.size()) {
        auto token = lex(source);
        Token::Print(token);
        printf("\n");
        if (token.kind == Token::Kind::EOF_)
            break;
        if (token.kind != Token::Kind::LIST) {
            printf("ERROR: Unexpected token at top level; this is LISP, so use lists!\n");
            return;
        }

        // First element of list should be an identifier that will help us to
        // parse this meaningfully into the build scenario.
        if (token.elements.empty() or token.elements[0].kind != Token::Kind::IDENTIFIER) {
            printf("ERROR: Expected identifier in operator position of top level list!\n");
            return;
        }
        const auto identifier = token.elements[0].identifier;

        // TARGET CREATION
        // "executable", "library", "target"
        if (identifier == "executable" or identifier == "library" or identifier == "target") {
            // TODO: Ensure second element is an identifier that doesn't already refer
            // to an existing target.

            printf("TODO: Create target\n");
            return;
        }

        // TARGET RELATED
        // "sources", "include-directories"
        if (identifier == "sources" or identifier == "include-directories") {
            // TODO: Ensure second element is an identifier that refers to an existing
            // target, and get a reference to that target so we can add a few details.
            return;
        }

        // TARGET REQUISITE REGISTRATION
        // "command", "copy", "dependency"
        if (identifier == "command" or identifier == "copy" or identifier == "dependency") {
            // TODO: Ensure second element is an identifier that refers to an existing
            // target, and get a reference to that target so we can register a new
            // requisite.
            return;
        }
    }
}

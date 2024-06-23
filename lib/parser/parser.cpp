#include <algorithm>
#include <cctype>
#include <format>
#include <string>
#include <string_view>
#include <vector>
#include <parser/parser.h>

// TODO: string syntax for identifiers with delimiters whitespace in them.
struct Token {
    enum Kind {
        UNKNOWN,
        EOF_,
        LIST,
        IDENTIFIER,
    } kind;

    std::string identifier;
    std::vector<Token> elements;

    static auto eof() -> Token {
        return {
            Token::Kind::EOF_, "", {}
        };
    }

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

constexpr bool token_is_eof(const Token &token) {
    return token.kind == Token::Kind::EOF_;
}
constexpr bool token_is_list(const Token &token) {
    return token.kind == Token::Kind::LIST;
}
constexpr bool token_is_identifier(const Token &token) {
    return token.kind == Token::Kind::IDENTIFIER;
}

#define LEX_LIST_BEGIN '('
#define LEX_LIST_END   ')'
#define LEX_LINE_COMMENT_BEGIN ';'
#define LEX_STRING_BEGIN '"'
#define LEX_STRING_END   '"'

bool isdelimiter(const char c) {
    return c == LEX_LIST_BEGIN
           or c == LEX_LIST_END
           or c == LEX_LINE_COMMENT_BEGIN;
}

bool lex_eat_comments(std::string_view &source) {
    bool ret{false};
    while (source.size() and source.data()[0] == LEX_LINE_COMMENT_BEGIN) {
        ret = true;
        // Eat everything up until newline.
        while (source.size() and source.data()[0] != '\n')
            source.remove_prefix(1);
        // Eat the newline.
        source.remove_prefix(1);
    }
    return ret;
}

// Returns true iff whitespace was eaten.
bool lex_eat_whitespace(std::string_view &source) {
    bool ret{false};
    while (source.size() and isspace(source.data()[0])) {
        ret = true;
        source.remove_prefix(1);
    }
    return ret;
}

auto lex(std::string_view& source) -> Token {
    Token out{};

    while (lex_eat_comments(source) or lex_eat_whitespace(source))
        ;

    // Cannot lex a token from an empty source.
    if (source.empty()) return Token::eof();

    // Get first character from source.
    const auto c = source.data()[0];
    // Eat it.
    source.remove_prefix(1);

    // Identifier
    if (c == LEX_STRING_BEGIN) {
        out.kind = Token::Kind::IDENTIFIER;
        while (source.size() and source.data()[0] != LEX_STRING_END) {
            // Add character that is not the initial string begin or string end symbol
            // to the string contents.
            out.identifier += source.data()[0];
            // Now eat it.
            source.remove_prefix(1);
        }
        // Handle "open string then EOF" case
        if (source.empty()) {
            printf("ERROR: Got EOF before string closing symbol (%c)\n", LEX_STRING_END);
            exit(1);
        }
        // Eat string end symbol.
        source.remove_prefix(1);
        return out;
    }
    // List
    if (c == LEX_LIST_BEGIN) {
        out.kind = Token::Kind::LIST;
        // Parse list contents (fancy fun LISP tunnels)
        while (source.size() and source.data()[0] != LEX_LIST_END) {
            out.elements.push_back(lex(source));
            // Since the loop condition checks the current character for being a list
            // end, we need to advance the current character past all whitespace and
            // comments after the end of a parsed element, that way we will be "up to"
            // the list end symbol and the comparison will actually work.
            while (lex_eat_comments(source) or lex_eat_whitespace(source))
                ;
        }

        if (source.empty()) {
            printf("ERROR: Got EOF before list closing symbol %c\n", LEX_LIST_END);
            exit(1);
        }

        // Eat list closing character.
        source.remove_prefix(1);

        return out;
    }
    // Identifier Part Two
    if (not isspace(c) and not isdelimiter(c)) {
        out.kind = Token::Kind::IDENTIFIER;
        // lex identifier
        out.identifier = std::string{c};
        // Until character is whitespace or delimiter...
        while (not isspace(source.data()[0]) and not isdelimiter(source.data()[0])) {
            // Add character to identifier.
            out.identifier += source.data()[0];
            // Now eat it.
            source.remove_prefix(1);
        }
        return out;
    }
    // Number
    if (isdigit(c)) {
        // TODO: lex number OR unreachable?
    }

    // TODO: unreachable
    printf("UNREACHABLE\n");
    printf("  char is %c\n", c);
    exit(2);
    return out;
}

auto parse(std::string_view source) -> BuildScenario {
    // The idea is this will parse the source into a list of actions to
    // perform (i.e. shell commands to run for targets and that sort of
    // thing).
    BuildScenario build_scenario{};
    while (source.size()) {
        auto token = lex(source);

        //Token::Print(token);
        //printf("\n");

        if (token_is_eof(token))
            break;
        if (not token_is_list(token)) {
            printf("ERROR: Unexpected token at top level; this is LISP, so use lists!\n");
            exit(1);
        }

        // First element of list should be an identifier that will help us to
        // parse this meaningfully into the build scenario.
        if (token.elements.empty() or not token_is_identifier(token.elements[0])) {
            printf("ERROR: Expected identifier in operator position of top level list!\n");
            exit(1);
        }
        const auto identifier = token.elements[0].identifier;

        // TARGET CREATION
        // "executable", "library", "target"
        if (identifier == "executable" or identifier == "library" or identifier == "target") {
            // Ensure second element is an identifier.
            if (token.elements.size() < 2 or
                not token_is_identifier(token.elements[1])) {
                printf("ERROR: Second element must be an identifier");
                exit(1);
            }
            std::string name = token.elements[1].identifier;

            // Ensure name doesn't already refer to an existing target.
            for (const auto &target : build_scenario.targets) {
                if (name == target.name) {
                    printf("ERROR: Targets must not share a name (hint: %s)\n", name.data());
                    exit(1);
                }
            }

            Target::Kind t_kind{};
            if (identifier == "target") t_kind = Target::Kind::GENERIC;
            else if (identifier == "executable") t_kind = Target::Kind::EXECUTABLE;
            else if (identifier == "library") t_kind = Target::Kind::LIBRARY;
            else {
                printf("ERROR: Unhandled target creation identifier %s\n", identifier.data());
                exit(1);
            }

            // Register target in BuildScenario.
            build_scenario.targets.push_back(Target::NamedTarget(t_kind, name));

            continue;
        }

        // TARGET RELATED
        // "sources", "include-directories", "defines", "flags" for executables and libraries
        else if (identifier == "sources" or identifier == "include-directories" or identifier == "flags" or identifier == "defines") {
            // Ensure second element is an identifier.
            if (token.elements.size() < 2 or
                not token_is_identifier(token.elements[1])) {
                printf("ERROR: Second element must be an identifier");
                exit(1);
            }
            std::string name = token.elements[1].identifier;
            // Ensure that identifier that refers to an existing target, and get a
            // reference to that target so we can add a few details.
            auto target = build_scenario.target(name);
            if (target == build_scenario.targets.end()) {
                printf("ERROR: Second element must refer to an existing target "
                       "(which \"%s\" does not)\n",
                       name.data());
                exit(1);
            }
            if (target->kind != Target::Kind::EXECUTABLE and
                target->kind != Target::Kind::LIBRARY) {
                printf("ERROR: %s is only applicable to executable and library "
                       "targets",
                       identifier.data());
                exit(1);
            }

            // Register sources in target
            if (identifier == "sources") {
                // Begin iterating all elements past target name.
                for (auto it = token.elements.begin() + 2;
                     it != token.elements.end(); it++) {
                    // TODO: Handle (directory-contents)
                    if (not token_is_identifier(*it)) {
                        printf("ERROR: Sources must be an identifier (just a file path)\n");
                        exit(1);
                    }
                    target->sources.push_back(it->identifier);
                }
            }

            // Register include directories in target
            else if (identifier == "include-directories") {
                // Begin iterating all elements past target name.
                for (auto it = token.elements.begin() + 2;
                     it != token.elements.end(); it++) {
                    // TODO: Handle (directory-contents)
                    if (not token_is_identifier(*it)) {
                        printf("ERROR: Include directories must be an identifier (just a file path)\n");
                        exit(1);
                    }
                    target->include_directories.push_back(it->identifier);
                }
            }

            else if (identifier == "defines") {
                // Begin iterating all elements past target name.
                for (auto it = token.elements.begin() + 2;
                     it != token.elements.end(); it++) {
                    if (not token_is_identifier(*it)) {
                        printf("ERROR: Defines must be identifiers\n");
                        exit(1);
                    }
                    target->defines.push_back(it->identifier);
                }
            }

            else if (identifier == "flags") {
                // Begin iterating all elements past target name.
                for (auto it = token.elements.begin() + 2;
                     it != token.elements.end(); it++) {
                    if (not token_is_identifier(*it)) {
                        printf("ERROR: Flags must be identifiers\n");
                        exit(1);
                    }
                    target->flags.push_back(it->identifier);
                }
            }

            else {
                printf("ERROR: Unhandled target related operator %s. Likely an "
                       "internal error, sorry.\n",
                       identifier.data());
                exit(1);
            }

            continue;
        }

        // TARGET REQUISITE REGISTRATION
        // "command", "copy", "dependency"
        else if (identifier == "command" or identifier == "copy" or identifier == "dependency") {
            // Ensure second element is an identifier.
            if (token.elements.size() < 2 or
                not token_is_identifier(token.elements[1])) {
                printf("ERROR: Second element must be an identifier");
                exit(1);
            }
            std::string name = token.elements[1].identifier;
            // Ensure that identifier that refers to an existing target, and get a
            // reference to that target so we can add a few details.
            auto target = build_scenario.target(name);
            if (target == build_scenario.targets.end()) {
                printf("ERROR: Second element must refer to an existing target "
                       "(which \"%s\" does not)\n",
                       name.data());
                exit(1);
            }

            auto requisite = Target::Requisite{};
            if (identifier == "command") {
                requisite.kind = Target::Requisite::Kind::COMMAND;
                // Ensure third element is an identifier.
                if (not token_is_identifier(token.elements[2])) {
                    printf("ERROR: command (after target name) must be an identifier\n");
                    exit(1);
                }
                requisite.text = token.elements[2].identifier;
                // Begin iterating all elements past target name and command.
                for (auto it = token.elements.begin() + 3;
                     it != token.elements.end(); it++) {
                    // TODO: Handle (directory-contents)
                    if (not token_is_identifier(*it)) {
                        printf("ERROR: command arguments must be an identifier\n");
                        exit(1);
                    }
                    requisite.arguments.push_back(it->identifier);
                }
            }
            else if (identifier == "copy") {
                requisite.kind = Target::Requisite::Kind::COPY;
                // TODO: Handle (directory ...), (directory-contents ...)
                // Ensure third element is an identifier.
                if (not token_is_identifier(token.elements[2])) {
                    printf("ERROR: copy source argument must be an identifier for now, sorry\n");
                    exit(1);
                }
                requisite.text = token.elements[2].identifier;
                // Ensure fourth element is an identifier.
                if (not token_is_identifier(token.elements[3])) {
                    printf("ERROR: copy destination argument must be an identifier for now, sorry\n");
                    exit(1);
                }
                requisite.text = token.elements[3].identifier;
            }
            else if (identifier == "dependency") {
                requisite.kind = Target::Requisite::Kind::DEPENDENCY;
                // Ensure third element is an identifier.
                if (not token_is_identifier(token.elements[2])) {
                    printf("ERROR: dependency target name must be an identifier\n");
                    exit(1);
                }
                // Ensure that identifier refers to an existing target.
                auto dep_target = build_scenario.target(token.elements[2].identifier);
                if (dep_target == build_scenario.targets.end()) {
                    printf("ERROR: dependency on target %s but that target doesn't exist\n", token.elements[2].identifier.data());
                    exit(1);
                }

                // If we are depending on a library target, link with it.
                if (dep_target->kind == Target::Kind::LIBRARY)
                    target->linked_libraries.push_back(dep_target->name);

                requisite.text = token.elements[2].identifier;
            }

            target->requisites.push_back(requisite);

            continue;
        }
    }
    //BuildScenario::Print(build_scenario);
    return build_scenario;
}

#include <algorithm>
#include <cctype>
#include <format>
#include <string>
#include <string_view>
#include <vector>

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
        source.remove_prefix(1);
        return out;
    }
    // Number
    if (isdigit(c)) {
        // TODO: lex number OR unreachable?
    }

    // TODO: unreachable
    return out;
}

struct Target {
    const std::string name;
    // Source code
    std::vector<std::string> sources;
    // For -I
    std::vector<std::string> include_directories;
    // For -L
    std::vector<std::string> linked_libraries;

    static void Print(const Target &target) {
        printf("TARGET %s\n", target.name.data());
        if (target.sources.size()) {
            printf("Sources:\n");
            for (const auto source : target.sources)
                printf("- %s\n", source.data());
        }
        if (target.include_directories.size()) {
            printf("Include Directories:\n");
            for (const auto include_dir : target.include_directories)
                printf("- %s\n", include_dir.data());
        }
        if (target.linked_libraries.size()) {
            printf("Linked Libraries:\n");
            for (const auto library : target.linked_libraries)
                printf("- %s\n", library.data());
        }
    }
};

struct BuildScenario {
    std::vector<Target> targets;

    static void Print(const BuildScenario& build_scenario) {
        for (const auto &target : build_scenario.targets)
            Target::Print(target);
    }
};

void parse(std::string_view source) {
    // TODO: The idea is this will parse the source into a list of actions to
    // perform (i.e. shell commands to run for targets and that sort of thing).
    BuildScenario build_scenario{};
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
            // Ensure second element is an identifier.
            if (token.elements.size() < 2 or
                token.elements[1].kind != Token::Kind::IDENTIFIER) {
                printf("ERROR: Second element must be an identifier");
                return;
            }
            std::string name = token.elements[1].identifier;

            // Ensure name doesn't already refer to an existing target.
            for (const auto &target : build_scenario.targets) {
                if (name == target.name) {
                    printf("ERROR: Targets must not share a name (hint: %s)\n", name.data());
                    return;
                }
            }

            Target t{name};

            // Register target in BuildScenario.
            build_scenario.targets.push_back(t);

            continue;
        }

        // TARGET RELATED
        // "sources", "include-directories"
        if (identifier == "sources" or identifier == "include-directories") {
            // Ensure second element is an identifier.
            if (token.elements.size() < 2 or
                token.elements[1].kind != Token::Kind::IDENTIFIER) {
                printf("ERROR: Second element must be an identifier");
                return;
            }
            std::string name = token.elements[1].identifier;
            // TODO: Ensure that identifier that refers to an existing target, and get
            // a reference to that target so we can add a few details.
            auto target = std::find_if(build_scenario.targets.begin(), build_scenario.targets.end(), [&] (const Target& t) {
                return t.name == name;
            });
            if (target == build_scenario.targets.end()) {
                printf("ERROR: Second element must refer to an existing target "
                       "(which \"%s\" does not)\n",
                       name.data());
                return;
            }

            if (identifier == "sources") {
                // Begin iterating all elements past target name.
                for (auto it = token.elements.begin() + 2;
                     it != token.elements.end(); it++) {
                    // TODO: Handle (directory-contents)
                    if (it->kind != Token::Kind::IDENTIFIER) {
                        printf("ERROR: Sources must be an identifier (just a file path)\n");
                        return;
                    }
                    target->sources.push_back(it->identifier);
                }
            }

            if (identifier == "include-directories") {
                // Begin iterating all elements past target name.
                for (auto it = token.elements.begin() + 2;
                     it != token.elements.end(); it++) {
                    // TODO: Handle (directory-contents)
                    if (it->kind != Token::Kind::IDENTIFIER) {
                        printf("ERROR: Include directories must be an identifier (just a file path)\n");
                        return;
                    }
                    target->include_directories.push_back(it->identifier);
                }
            }

            continue;
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
    BuildScenario::Print(build_scenario);
}

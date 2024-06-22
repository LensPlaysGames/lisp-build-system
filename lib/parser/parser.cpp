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
    enum Kind {
      UNKNOWN,
      GENERIC,
      LIBRARY,
      EXECUTABLE,
    } kind;

    const std::string name;
    // Source code
    std::vector<std::string> sources;
    // For -I
    std::vector<std::string> include_directories;
    // For -L
    std::vector<std::string> linked_libraries;

    struct Requisite {
        enum Kind {
          COMMAND,
          COPY,
          DEPENDENCY,
        } kind;

        std::string text;
        std::vector<std::string> arguments;
        std::string destination;

        static void Print(const Requisite& requisite) {
            switch (requisite.kind) {
            case DEPENDENCY:
                printf("build dependency %s", requisite.text.data());
                break;
            case COMMAND:
                printf("%s", requisite.text.data());
                for (const auto &arg : requisite.arguments)
                    printf(" %s", arg.data());
                break;
            case COPY:
                printf("copy %s %s", requisite.text.data(), requisite.destination.data());
                break;
            }
        }
    };

    std::vector<Requisite> requisites;

    static void Print(const Target &target) {
        switch (target.kind) {
        case UNKNOWN: printf("UNKNOWN-KIND TARGET "); break;
        case GENERIC: printf("TARGET "); break;
        case LIBRARY: printf("LIBRARY "); break;
        case EXECUTABLE: printf("EXECUTABLE "); break;
            break;
        }
        printf("%s\n", target.name.data());
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
        if (target.requisites.size()) {
            printf("Requisites:\n");
            for (const auto requisite : target.requisites) {
                printf("- ");
                Requisite::Print(requisite);
                printf("\n");
            }
        }
    }
};

struct Compiler {
    const std::string library_template{};
    const std::string executable_template{};
};

struct BuildScenario {
    std::vector<Target> targets;

    static void Print(const BuildScenario& build_scenario) {
        for (const auto &target : build_scenario.targets)
            Target::Print(target);
    }

    static void Commands(const BuildScenario &build_scenario,
                         std::string target_name, Compiler compiler,
                         std::string indent = "") {
        printf("%sCOMMANDS FOR TARGET %s:\n", indent.data(), target_name.data());

        auto target = std::find_if(build_scenario.targets.begin(), build_scenario.targets.end(), [&] (const Target& t) {
            return t.name == target_name;
        });
        if (target == build_scenario.targets.end()) {
            printf("ERROR: Target %s does not exist in build scenario", target_name.data());
            return;
        }
        for (const auto &requisite : target->requisites) {
            switch (requisite.kind) {
            case Target::Requisite::COMMAND:
                printf("%s%s", indent.data(), requisite.text.data());
                for (const auto &arg : requisite.arguments)
                    printf(" %s", arg.data());
                printf("\n");
                break;
            case Target::Requisite::COPY:
                printf("%scp %s %s\n", indent.data(), requisite.text.data(), requisite.destination.data());
                break;
            case Target::Requisite::DEPENDENCY:
                // FIXME: what compiler to use for dependency.
                BuildScenario::Commands(build_scenario, requisite.text, compiler, indent + "    ");
                break;
            }
        }
        if (target->kind == Target::Kind::EXECUTABLE) {
            std::string build_executable_command{};
            for (auto i = 0; i < compiler.executable_template.size(); ++i) {
                const char c = compiler.executable_template.data()[i];
                switch (c) {

                case '%': {
                    if (i + 1 >= compiler.executable_template.size()) {
                        build_executable_command += c;
                        break;
                    }
                    const char nextc = compiler.executable_template.data()[++i];
                    if (nextc == 'i') {
                        bool notfirst{false};
                        for (const auto& source : target->sources) {
                            if (notfirst) build_executable_command += ' ';
                            build_executable_command += source;
                            notfirst = true;
                        }
                    } else if (nextc == 'o') {
                        build_executable_command += target_name;
#ifdef _WIN32
                        build_executable_command += ".exe";
#endif
                    } else {
                        printf("ERROR: Unrecognized format specifier in "
                               "compiler template string\n"
                               "    format specifier: %c\n"
                               "    template string: %s\n",
                               nextc,
                               compiler.executable_template.data());
                    }
                } break;

                default:
                    build_executable_command += c;
                    break;
                }
            }
            printf("%s%s\n", indent.data(), build_executable_command.data());
        } else if (target->kind == Target::Kind::LIBRARY) {
            std::string build_library_command{};
            for (auto i = 0; i < compiler.library_template.size(); ++i) {
                const char c = compiler.library_template.data()[i];
                switch (c) {

                case '%': {
                    if (i + 1 >= compiler.library_template.size()) {
                        build_library_command += c;
                        break;
                    }
                    const char nextc = compiler.library_template.data()[++i];
                    if (nextc == 'i') {
                        bool notfirst{false};
                        for (const auto& source : target->sources) {
                            if (notfirst) build_library_command += ' ';
                            build_library_command += source;
                            notfirst = true;
                        }
                    } else if (nextc == 'o') {
                        build_library_command += target_name;
#ifdef _WIN32
                        build_library_command += ".exe";
#endif
                    } else {
                        printf("ERROR: Unrecognized format specifier in "
                               "compiler template string\n"
                               "    format specifier: %c\n"
                               "    template string: %s\n",
                               nextc,
                               compiler.library_template.data());
                    }
                } break;

                default:
                    build_library_command += c;
                    break;
                }
            }
            printf("%s%s\n", indent.data(), build_library_command.data());
        } else {
            printf("ERROR: Unhandled target kind %d in BuildScenario::Commands(), sorry\n", target->kind);
        }
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

            Target::Kind t_kind = Target::Kind::GENERIC;
            if (identifier == "executable") t_kind = Target::Kind::EXECUTABLE;
            else if (identifier == "library") t_kind = Target::Kind::LIBRARY;
            Target t{t_kind, name};

            // Register target in BuildScenario.
            build_scenario.targets.push_back(t);

            continue;
        }

        // TARGET RELATED
        // "sources", "include-directories"
        else if (identifier == "sources" or identifier == "include-directories") {
            // Ensure second element is an identifier.
            if (token.elements.size() < 2 or
                token.elements[1].kind != Token::Kind::IDENTIFIER) {
                printf("ERROR: Second element must be an identifier");
                return;
            }
            std::string name = token.elements[1].identifier;
            // Ensure that identifier that refers to an existing target, and get a
            // reference to that target so we can add a few details.
            auto target = std::find_if(build_scenario.targets.begin(), build_scenario.targets.end(), [&] (const Target& t) {
                return t.name == name;
            });
            if (target == build_scenario.targets.end()) {
                printf("ERROR: Second element must refer to an existing target "
                       "(which \"%s\" does not)\n",
                       name.data());
                return;
            }

            // Register sources in target
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

            // Register include directories in target
            else if (identifier == "include-directories") {
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
        else if (identifier == "command" or identifier == "copy" or identifier == "dependency") {
            // Ensure second element is an identifier.
            if (token.elements.size() < 2 or
                token.elements[1].kind != Token::Kind::IDENTIFIER) {
                printf("ERROR: Second element must be an identifier");
                return;
            }
            std::string name = token.elements[1].identifier;
            // Ensure that identifier that refers to an existing target, and get a
            // reference to that target so we can add a few details.
            auto target = std::find_if(build_scenario.targets.begin(), build_scenario.targets.end(), [&] (const Target& t) {
                return t.name == name;
            });
            if (target == build_scenario.targets.end()) {
                printf("ERROR: Second element must refer to an existing target "
                       "(which \"%s\" does not)\n",
                       name.data());
                return;
            }

            auto requisite = Target::Requisite{};
            if (identifier == "command") {
                requisite.kind = Target::Requisite::Kind::COMMAND;
                // TODO: Ensure third element is an identifier.
                requisite.text = token.elements[2].identifier;
                // Begin iterating all elements past target name and command.
                for (auto it = token.elements.begin() + 3;
                     it != token.elements.end(); it++) {
                    // TODO: Handle (directory-contents)
                    if (it->kind != Token::Kind::IDENTIFIER) {
                        printf("ERROR: command arguments must be an identifier\n");
                        return;
                    }
                    requisite.arguments.push_back(it->identifier);
                }
            }
            else if (identifier == "copy") {
                requisite.kind = Target::Requisite::Kind::COPY;
                // TODO: Ensure third element is an identifier.
                requisite.text = token.elements[2].identifier;
                // TODO: Ensure fourth element is an identifier.
                requisite.text = token.elements[3].identifier;
            }
            else if (identifier == "dependency") {
                requisite.kind = Target::Requisite::Kind::DEPENDENCY;
                // TODO: Ensure third element is an identifier.
                // TODO: Ensure that identifier refers to an existing target.
                requisite.text = token.elements[2].identifier;
            }

            target->requisites.push_back(requisite);

            continue;
        }
    }
    BuildScenario::Print(build_scenario);
    BuildScenario::Commands(build_scenario, "lbs", {
        "c++ -c %i -o %o",
        "c++ %i -o %o"
    });
}

// Compiler:
// - Object Compilation Template with %o (output filename) and %i
//   (input source filename), probably eventually flags, defines, etc.
//   "cc -c %i -o %o"
// - Executable Compilation Template with %o (output filename),
//   %i (input object(s)).
//   "cc %i -o %o"
// Using a BuildScenario and these templates, we should be able to produce
// build commands.

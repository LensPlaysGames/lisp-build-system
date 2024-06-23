#ifndef LBS_PARSER_H
#define LBS_PARSER_H

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

// Compiler:
// - Object Compilation Template with %o (output filename) and %i
//   (input source filename), probably eventually flags, defines, etc.
//   "cc -c %i -o %o"
// - Executable Compilation Template with %o (output filename),
//   %i (input object(s)).
//   "cc %i -o %o"
// Using a BuildScenario and these templates, we should be able to produce
// build commands.
struct Compiler {
    const std::string library_template{};
    const std::string executable_template{};
};

struct Target {
    enum Kind {
      UNKNOWN,
      GENERIC,
      LIBRARY,
      EXECUTABLE,
    } kind;

    const std::string name;
    std::vector<std::string> sources;
    std::vector<std::string> include_directories;
    std::vector<std::string> linked_libraries;
    std::vector<std::string> flags;
    std::vector<std::string> defines;

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
                for (const auto& arg : requisite.arguments)
                    printf(" %s", arg.data());
                break;
            case COPY:
                printf("copy %s %s", requisite.text.data(), requisite.destination.data());
                break;
            }
        }
    };

    std::vector<Requisite> requisites;

    static auto NamedTarget(Target::Kind kind, std::string name)
        -> Target {
        return Target {
            kind, std::move(name), {}, {}, {}, {}, {}, {}
        };
    }

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
            for (const auto& source : target.sources)
                printf("- %s\n", source.data());
        }
        if (target.include_directories.size()) {
            printf("Include Directories:\n");
            for (const auto& include_dir : target.include_directories)
                printf("- %s\n", include_dir.data());
        }
        if (target.linked_libraries.size()) {
            printf("Linked Libraries:\n");
            for (const auto& library : target.linked_libraries)
                printf("- %s\n", library.data());
        }
        if (target.requisites.size()) {
            printf("Requisites:\n");
            for (const auto& requisite : target.requisites) {
                printf("- ");
                Requisite::Print(requisite);
                printf("\n");
            }
        }
    }
};

// FIXME: We should move this to lib/compiler/compiler.cpp and also make
// inc/compiler/compiler.h a thing... For now it's static.
static auto compiler_format(std::string format, const Target& target) -> std::string {
    const std::string output_name = target.name
#ifdef _WIN32
    + ".exe"
#endif
            ;

    std::string build_command{};

    // For emitting warnings.
    bool format_has_input{false};
    bool format_has_output{false};
    bool format_has_flags{false};
    bool format_has_defines{false};

    for (size_t i = 0; i < format.size(); ++i) {
        const char c = format.data()[i];
        switch (c) {

        case '%': {
            if (i + 1 >= format.size()) {
                build_command += c;
                break;
            }

            const char nextc = format.data()[++i];
            switch (nextc) {
            case 'i': {
                format_has_input = true;
                bool notfirst{false};
                for (const auto& source : target.sources) {
                    if (notfirst) build_command += ' ';
                    build_command += source;
                    notfirst = true;
                }
            } break;

            case 'o': {
                format_has_output = true;
                build_command += output_name;
            } break;

            case 'f': {
                format_has_flags = true;
                bool notfirst{false};
                for (const auto& flag : target.flags) {
                    if (notfirst) build_command += ' ';
                    build_command += flag;
                    notfirst = true;
                }
            } break;

            case 'd': {
                format_has_defines = true;
                bool notfirst{false};
                for (const auto& define : target.defines) {
                    if (notfirst) build_command += ' ';
                    build_command += define;
                    notfirst = true;
                }
            } break;

            default:
                printf("ERROR: Unrecognized format specifier in "
                       "compiler template string\n"
                       "    format specifier: %c\n"
                       "    template string: %s\n",
                       nextc, format.data());
                exit(1);
                break;
            }
        } break;

        default:
            build_command += c;
            break;
        }
    }
    if (not format_has_input) {
        printf(
               "WARNING: Executable format string for compiler does not have input "
               "format specifier, and likely will not work without it.\n");
    }
    if (not format_has_output) {
        printf(
               "WARNING: Executable format string for compiler does not have output "
               "format specifier, and likely will not work without it.\n");
    }
    if (not format_has_flags) {
        printf(
               "WARNING: Executable format string for compiler does not have flags "
               "format specifier, and may not work without it.\n");
    }
    if (not format_has_defines) {
        printf(
               "WARNING: Executable format string for compiler does not have defines "
               "format specifier, and may not work without it.\n");
    }

    return build_command;
}

struct BuildScenario {
    std::vector<Target> targets;

    // Check return value against targets.end()
    auto target(const std::string_view name) {
        return std::find_if(targets.begin(), targets.end(), [&] (const Target& t) {
            return t.name == name;
        });
    };

    auto target(const std::string_view name) const {
        return std::find_if(targets.begin(), targets.end(), [&] (const Target& t) {
            return t.name == name;
        });
    };

    static void Print(const BuildScenario& build_scenario) {
        for (const auto &target : build_scenario.targets)
            Target::Print(target);
    }

    struct BuildCommands {
        std::vector<std::string> commands;
        std::vector<std::string> artifacts;

        void push_back(std::string new_command) {
            commands.push_back(new_command);
        }
        void push_back(BuildCommands new_build_commands) {
            commands.insert(commands.end(), new_build_commands.commands.begin(), new_build_commands.commands.end());
            artifacts.insert(artifacts.end(), new_build_commands.artifacts.begin(), new_build_commands.artifacts.end());
        }

        auto as_one_command(const std::string_view separator = " && ") -> std::string {
            std::string out_command{};
            bool notfirst{false};
            for (const auto& command : commands) {
                if (notfirst) out_command += separator;
                out_command += command;
                notfirst = true;
            }
            return out_command;
        }
    };

    static auto Commands(const BuildScenario &build_scenario,
                         std::string target_name, Compiler compiler) -> BuildCommands {
        auto target = build_scenario.target(target_name);
        if (target == build_scenario.targets.end()) {
            printf("ERROR: Target %s does not exist in build scenario", target_name.data());
            return {};
        }
        BuildCommands build_commands{};
        for (const auto& requisite : target->requisites) {
            switch (requisite.kind) {
            case Target::Requisite::COMMAND: {
                std::string command{requisite.text};
                for (const auto& arg : requisite.arguments){
                    command += ' ';
                    command += arg;
                }
                build_commands.push_back(command);
            } break;
            case Target::Requisite::COPY: {
                // TODO: Handle (directory), (directory-contents)
                // TODO: Record artifact(s)
                std::string copy_command{"cp "};
                copy_command += requisite.text;
                copy_command += ' ';
                copy_command += requisite.destination;
                build_commands.push_back(copy_command);
            } break;
            case Target::Requisite::DEPENDENCY:
                // FIXME: what compiler to use for dependency.
                build_commands.push_back(BuildScenario::Commands(build_scenario, requisite.text, compiler));
                break;
            }
        }
        if (target->kind == Target::Kind::EXECUTABLE or target->kind == Target::Kind::LIBRARY) {
            // Record artifact(s)
            build_commands.artifacts.push_back(target_name);

            auto build_command = compiler_format(target->kind == Target::Kind::EXECUTABLE ? compiler.executable_template : compiler.library_template, *target);

            // Include directories.
            for (const auto& include_dir : target->include_directories) {
                build_command += " -I";
                build_command += include_dir;
            }

            // Linked libraries.
            for (const auto& library_name : target->linked_libraries) {
                build_command += ' ';
                build_command += library_name;
            }

            build_commands.push_back(build_command);
        } else {
            printf("ERROR: Unhandled target kind %d in BuildScenario::Commands(), sorry\n", target->kind);
            exit(1);
        }

        return build_commands;
    }
};

auto parse(std::string_view source) -> BuildScenario;

#endif /* LBS_PARSER_H */

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
    // TODO: flags, defines

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
        if (target->kind == Target::Kind::EXECUTABLE) {
            const std::string output_name = target_name
#ifdef _WIN32
            + ".exe"
#endif
            ;
            // Record artifact(s)
            build_commands.artifacts.push_back(target_name);

            std::string build_executable_command{};
            for (size_t i = 0; i < compiler.executable_template.size(); ++i) {
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
                        build_executable_command += output_name;
                    } else {
                        printf("ERROR: Unrecognized format specifier in "
                               "compiler template string\n"
                               "    format specifier: %c\n"
                               "    template string: %s\n",
                               nextc, compiler.executable_template.data());
                        exit(1);
                    }
                } break;

                default:
                    build_executable_command += c;
                    break;
                }
            }

            // Include directories.
            for (const auto& include_dir : target->include_directories) {
                build_executable_command += " -I";
                build_executable_command += include_dir;
            }

            // Linked libraries.
            for (const auto& library_name : target->linked_libraries) {
                build_executable_command += " ";
                build_executable_command += library_name;
            }

            build_commands.push_back(build_executable_command);
        } else if (target->kind == Target::Kind::LIBRARY) {
            // Record artifact(s)
            build_commands.artifacts.push_back(target_name);

            std::string build_library_command{};
            for (size_t i = 0; i < compiler.library_template.size(); ++i) {
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

            for (const auto& include_dir : target->include_directories) {
                build_library_command += " -I";
                build_library_command += include_dir;
            }

            for (const auto& library_name : target->linked_libraries) {
                build_library_command += " ";
                build_library_command += library_name;
            }

            build_commands.push_back(build_library_command);
        } else {
            printf("ERROR: Unhandled target kind %d in BuildScenario::Commands(), sorry\n", target->kind);
        }

        return build_commands;
    }
};

auto parse(std::string_view source) -> BuildScenario;

#endif /* LBS_PARSER_H */

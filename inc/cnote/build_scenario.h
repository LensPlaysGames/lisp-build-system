#ifndef LBS_BUILD_SCENARIO_H
#define LBS_BUILD_SCENARIO_H

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include <cnote/target.h>

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

// FIXME: We should move this to lib/cnote/compiler.cpp and also make
// inc/cnote/compiler.h a thing, or something... For now it's static.
static auto expand_compiler_format(std::string format, const Target& target) -> std::string {
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
    std::vector<std::string> targets_built;

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

    bool target_built(const std::string_view name) const {
        auto found = std::find(targets_built.begin(), targets_built.end(), name);
        return found != targets_built.end();
    }
    void mark_target_built(std::string name) {
        if (target_built(name)) {
            printf("ERROR: Marking target %s as built but it has already been marked as such.\n", name.data());
            exit(1);
        }
        targets_built.push_back(std::move(name));
    }
    void mark_target_built(const Target &t) {
        mark_target_built(t.name);
    }

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

    static auto Commands(BuildScenario& build_scenario,
                         std::string target_name, Compiler compiler) -> BuildCommands {
        // Deduplication: don't build something already built.
        if (build_scenario.target_built(target_name)) return {};

        // Ensure target exists, and get a reference to it.
        auto target = build_scenario.target(target_name);
        if (target == build_scenario.targets.end()) {
            printf("ERROR: Target %s does not exist in build scenario", target_name.data());
            return {};
        }

        // Deduplication: mark target as having been built.
        build_scenario.mark_target_built(target_name);

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
                std::string copy_command{"cp "};
                copy_command += requisite.text;
                copy_command += ' ';
                copy_command += requisite.destination;
                build_commands.push_back(copy_command);
                // Record artifact(s)
                build_commands.artifacts.push_back(requisite.destination);
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

            auto build_command = expand_compiler_format(target->kind == Target::Kind::EXECUTABLE ? compiler.executable_template : compiler.library_template, *target);

            // Include directories.
            for (const auto& include_dir : target->include_directories) {
                build_command += " -I";
                build_command += include_dir;
            }

            // Linked libraries.
            if (target->kind == Target::Kind::EXECUTABLE) {
                for (const auto& library_name : target->linked_libraries) {
                    build_command += ' ';
                    build_command += library_name;
                }
            }

            build_commands.push_back(build_command);
        } else {
            printf("ERROR: Unhandled target kind %d in BuildScenario::Commands(), sorry\n", target->kind);
            exit(1);
        }

        return build_commands;
    }
};

#endif // #ifndef LBS_BUILD_SCENARIO_H

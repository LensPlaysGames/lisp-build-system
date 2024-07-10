#ifndef LBS_COMPILER_H
#define LBS_COMPILER_H

#include <string>

#include <lbs/target.h>

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
    const std::string name;
    const std::string object_template{};
    const std::string executable_template{};
};

// FIXME: We should move this to lib/lbs/compiler.cpp, or something...
// For now it's static.
static auto expand_compiler_format(std::string format, const Target& target)
    -> std::string {
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
                printf(
                    "ERROR: Unrecognized format specifier in "
                    "compiler template string\n"
                    "    format specifier: %c\n"
                    "    template string: %s\n",
                    nextc, format.data()
                );
                exit(1);
                break;
            }
        } break;

        default: build_command += c; break;
        }
    }
    if (not format_has_input) {
        printf(
            "WARNING: Executable format string for compiler does not have "
            "input "
            "format specifier, and likely will not work without it.\n"
        );
    }
    if (not format_has_output) {
        printf(
            "WARNING: Executable format string for compiler does not have "
            "output "
            "format specifier, and likely will not work without it.\n"
        );
    }
    if (not format_has_flags) {
        printf(
            "WARNING: Executable format string for compiler does not have "
            "flags "
            "format specifier, and may not work without it.\n"
        );
    }
    if (not format_has_defines) {
        printf(
            "WARNING: Executable format string for compiler does not have "
            "defines "
            "format specifier, and may not work without it.\n"
        );
    }

    return build_command;
}

#endif  // LBS_COMPILER_H

#include <string>
#include <filesystem>
#include <cstdio>
#include <cstdlib>

#include <parser/parser.h>
#include <tests/tests.h>

auto get_file_contents_or_exit(const std::string_view path) -> std::string {
    auto f = fopen(path.data(), "rb");
    if (not f) {
        printf("ERROR: Cannot get contents of file at %s\n", path.data());
        exit(1);
    }

    // Get file size.
    fseek(f, 0, SEEK_END); // seek to end of file
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET); // seek to beginning of file

    // Allocate string big enough to hold file contents.
    std::string contents;
    contents.resize(file_size);

    // Read data into string.
    fread(contents.data(), file_size, 1, f);
    fclose(f);

    return contents;
}

struct Options {
    std::vector<std::string> targets_to_build{};
    bool dry_run{false};
    bool clean_intermediates{true};
    bool just_clean{false};
};

int main(int argc, const char **argv) {
#ifdef LBS_TEST
    tests_run();
#endif

    Options options{};

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            const std::string_view arg{argv[i]};

            if (arg.substr(0, 2) == "-h" or arg.substr(0, 3) == "--h") {
                printf("USAGE: %s [FLAGS] [TARGETS...]\n", argv[0]);

                printf("FLAGS:\n");
                printf("  -n, --dry-run :: Only print, don't \"do\" anything.\n");
                printf("  --distclean :: Only delete build artifacts.\n");
                printf("  --noclean :: Do not delete intermediate files after build is completed.\n");

            }

            if (arg == "--dry-run" or arg == "-n")
                options.dry_run = true;

            else if (arg == "--distclean")
                options.just_clean = true;

            else if (arg == "--noclean")
                options.clean_intermediates = false;

            // NOTE: If you want a target that starts with a dash, you can change
            // these lines yourself :).
            else if (arg.size() and arg.data()[0] == '-'){
                printf("ERROR: Unknown command line argument \"%s\"\n",
                       arg.data());
                exit(1);
            }

            // All other command line arguments treated as targets to build
            else options.targets_to_build.push_back(std::string{arg});
        }
    }

    const char* path = ".lbs";
    if (not std::filesystem::exists(path)) {
        printf("No build file at .lbs found, exiting\n");
        printf("    To learn how to write one, see https://github.com/LensPlaysGames/lisp-build-system\n");
        return 1;
    }
    std::string source = get_file_contents_or_exit(path);
    auto build_scenario = parse(source);

    auto compiler = Compiler{
        "c++ -c %f %d %i -o %o",
        "c++ %f %d %i -o %o"
    };

    BuildScenario::BuildCommands build_commands{};

    std::string target_to_build{""};
    if (options.targets_to_build.size()) {
        for (const auto &target_to_build : options.targets_to_build)
            build_commands.push_back(BuildScenario::Commands(build_scenario, target_to_build, compiler));
    } else {
        // Attempt to find a single executable target, and build that by default.
        const Target* single_executable_target{nullptr};
        for (const auto &target : build_scenario.targets) {
            if (target.kind == Target::Kind::EXECUTABLE) {
                if (single_executable_target) {
                    single_executable_target = nullptr;
                    break;
                }
                single_executable_target = &target;
            }
        }
        if (not single_executable_target) {
            printf("ERROR: No targets provided on command line and a single executable target was not found to build by default\n");
            exit(1);
        }
        build_commands.push_back(BuildScenario::Commands(build_scenario, single_executable_target->name, compiler));
    }

    if (options.just_clean) {
        for (auto artifact : build_commands.artifacts) {
            if (options.dry_run)
                printf("REMOVE ARTIFACT at %s\n", artifact.data());
            else std::remove(artifact.data());
        }
        return 0;
    }

    // Execute build commands.
    for (const auto &command : build_commands.commands) {
        if (options.dry_run) {
            printf("%s\n", command.data());
        } else {
            // THIS IS NOT A FIRE DRILL THIS IS THE REAL DEAL
            // Use system() from libc to run build commands.
            auto rc = std::system(command.data());
            if (rc) {
                printf("[BUILD]:ERROR: command failed with status %d\n    %s\n", int(rc), command.data());
                break;
            }
        }
    }

    // To clean up the intermediates, we remove all artifacts except the last.
    // While this isn't guaranteed to work, it's pretty damn close.
    if (options.clean_intermediates) {
        for (auto it = build_commands.artifacts.begin();
             it != build_commands.artifacts.end() and
             it != build_commands.artifacts.end() - 1;
             ++it) {
            if (options.dry_run)
                printf("REMOVE ARTIFACT at %s\n", it->data());
            else std::remove(it->data());
        }
    }

    return 0;
}

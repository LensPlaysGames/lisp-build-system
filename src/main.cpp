#include <string>
#include <filesystem>
#include <cstdio>

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

int main() {
#ifdef LBS_TEST
    tests_run();
#endif

    const char* path = ".lbs";
    if (not std::filesystem::exists(path)) {
        printf("No build file at .lbs found, exiting\n");
        printf("    To learn how to write one, see https://github.com/LensPlaysGames/lisp-build-system\n");
        return 1;
    }
    std::string source = get_file_contents_or_exit(path);
    auto build_scenario = parse(source);

    auto build_commands = BuildScenario::Commands(build_scenario, "lbs", {
        "c++ -c %f %d %i -o %o",
        "c++ %f %d %i -o %o"
    });

    // Just print the command(s) for now.
    auto flattened_command = build_commands.as_one_command();
    printf("%s\n", flattened_command.data());

    return 0;
}

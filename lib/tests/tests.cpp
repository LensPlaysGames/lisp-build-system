#include <tests/tests.h>

#include <parser/parser.h>
#include <cstdio>

struct TestReturnValue {
  bool success{};
  std::string error{};
};

using TestFunctionType = const TestReturnValue (*)();

struct TestFunction {
    std::string name;
    TestFunctionType function;
};

/// ==BEGIN== PARSER TESTS
auto test_libparser_empty() -> const TestReturnValue {
  auto build_scenario = parse("");
  // For now, the parser exit(1)'s upon any problems, so if we get here we
  // are good.
  return {true};
}
// TODO: To better write more tests, we need to not just exit(1) when the
// parser errors and instead return a meaningful error value. This is
// fine, however the problem lies in that C++ is still stuck in 1975 and
// doesn't have string interpolation and formatting figured out yet.
/// ==FINAL== PARSER TESTS

void tests_run() {
    const std::vector<TestFunction> tests{
        {"libparser.empty", test_libparser_empty},
    };
    size_t failed{0};
    size_t succeeded{0};
    for (const auto &test : tests) {
        auto ret = test.function();
        if (ret.success) {
            ++succeeded;
        } else {
            ++failed;
            printf("[FAIL]: %s() | %s\n", test.name.data(), ret.error.data());
        }
    }
    printf("\nTEST RESULTS:\n");
    printf("  %zu SUCCESS\n", succeeded);
    printf("  %zu FAIL\n", failed);
    printf("  %zu TOTAL\n", tests.size());
}

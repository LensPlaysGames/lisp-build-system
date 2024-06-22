#include <string>

#include <parser/parser.h>

int main() {
    std::string source{"(executable foo) (sources foo src/main.cpp)"};
    parse(source);
    return 0;
}

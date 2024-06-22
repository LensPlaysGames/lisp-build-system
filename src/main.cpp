#include <string>

#include <parser/parser.h>

int main() {
    std::string source{"(executable foo) (sources foo src/main.cpp) (command foo echo suck it lol)"};
    parse(source);
    return 0;
}

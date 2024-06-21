#include <string>

#include <parser/parser.h>

int main() {
    std::string source{"(executable foo)"};
    parse(source);
    return 0;
}

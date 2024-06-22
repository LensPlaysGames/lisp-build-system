#include <string>

#include <parser/parser.h>

int main() {
    //std::string source{"(executable foo) (sources foo src/main.cpp) (command foo echo suck it lol)"};
    std::string source{"(library libparser)(sources libparser lib/parser/parser.cpp)  (executable lbs)(sources lbs src/main.cpp)(include-directories lbs inc)(dependency lbs libparser)"};
    parse(source);
    return 0;
}

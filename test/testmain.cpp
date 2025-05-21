#include <dlexer/regex.hpp>

using namespace dlexer;

int main() {
    RegexLexer l("([а-я]+)$|d");
    l.generateCProgram("main.c");
}

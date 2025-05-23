#include <dlexer/regex.hpp>

using namespace dlexer;

int main() {
    RegexLexer l("([а-я]+)|([a-z]+)|([0-9]+)");
    l.generateCProgram("main.c");
}

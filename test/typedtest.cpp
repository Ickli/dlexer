#include <dlexer/typed.hpp>
#include "common.hpp"
#include <iostream>

using namespace dlexer;

int main() {
    LexerTestCase t = LexerTestCase::create(
        "word \"abc\"",
        "abc abc",
        "abc", "abc"
    );
    int fail = t.testAndLog<TypedLexer>();

    t = LexerTestCase::create(
        "word \"abc\" space \" \"",
        "abc abc",
        "abc", " ", "abc"
    );
    fail |= t.testAndLog<TypedLexer>();

    t = LexerTestCase::create(
        "word \"abc\" space \" \" capword \"ABC\"",
        "abc abcABC",
        "abc", " ", "abc", "ABC"
    );
    fail |= t.testAndLog<TypedLexer>();

    t = LexerTestCase::create(
        "word \"abc\" space \" \" capwordquoted \"ABC\\\"\"",
        "abc abcABC\"",
        "abc", " ", "abc", "ABC\""
    );
    fail |= t.testAndLog<TypedLexer>();

    t = LexerTestCase::create(
        "word \"абв\" space \" \" capwordquoted \"АБВ\\\"\"",
        "абв абвАБВ\"",
        "абв", " ", "абв", "АБВ\""
    );
    fail |= t.testAndLog<TypedLexer>();

    return fail;
}

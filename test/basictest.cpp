#include <dlexer/basic.hpp>
#include "common.hpp"
#include <sstream>

using namespace dlexer;

int main() {
    LexerTestCase t = LexerTestCase::create(
        " ",
        "abc",
        "abc"
    );
    int fail = t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        " ",
        "abc abc",
        "abc", "abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        " <",
        "abc abc",
        "abc ", "abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        " >",
        "abc abc",
        "abc", " abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        " \\>",
        "abc abc",
        "abc", "abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        " \\>",
        ">>>abc abc",
        "abc", "abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        "d",
        "abc abc",
        "abc abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        " c",
        "abc abc",
        "ab", "ab"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        "c",
        "abc abc",
        "ab", " ab"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        " >",
        " abc abc",
        " abc", " abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        "\\\\>",
        "abc\\abc",
        "abc", "\\abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    t = LexerTestCase::create(
        "\\\\>\"!",
        "abc\\\"abc",
        "abc", "\\\"", "abc"
    );
    fail |= t.testAndLog<BasicLexer>();

    return fail;
}

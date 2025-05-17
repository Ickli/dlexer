#include <dlexer/regex.hpp>
#include "common.hpp"

using namespace dlexer;

int main() {
    LexerTestCase t = LexerTestCase::create(
        "a",
        "a",
        "a"
    );

    int fail = t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "a",
        "aa",
        "a", "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "aa",
        "aa",
        "aa"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "b",
        "ab",
        "b"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "b",
        "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "ab",
        "aab",
        "ab"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "a|b",
        "ab",
        "a", "b"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "ab|ba",
        "aba abba",
        "ab", "ab", "ba"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "a*",
        "a",
        "a", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "a*",
        "aa",
        "aa", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "a*",
        "a aa aaa",
        "a", "", "aa", "", "aaa", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "ba*",
        "a ba baa",
        "ba", "baa"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "a*",
        " ",
        "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(a*)",
        " ",
        "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(a)*",
        " ",
        "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(a)*",
        "a",
        "a", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(ab)*",
        "abab",
        "abab", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "aa|(ab)*",
        "ababaa",
        "abab", "aa", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "aa|(a|b)*",
        "ababaa",
        "abab", "aa", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(a|b)*|aa",
        "ababaa",
        "abab", "a", "a", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    return fail;
}

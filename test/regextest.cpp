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
        "(a|b|c)",
        "abc",
        "a", "b", "c"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(a|b|c)*",
        "abc",
        "abc", ""
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
        "(ab)+",
        "abab",
        "abab"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(ab)+",
        "ab",
        "ab"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(aa)+",
        "a aa aaa",
        "aa", "aa"
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
        "ababaa", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(a|b)*|aa",
        "ababaa",
        "ababaa", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(ab)*|aa",
        "ababaa",
        "abab", "", "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "^a",
        "aa",
        "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "^a|a",
        "aa",
        "a", "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "^a|ba",
        "aba",
        "a", "ba"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(^a)|ba",
        "aba",
        "a", "ba"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "^(a)|ba",
        "aba",
        "a", "ba"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "^(a)",
        "a\na",
        "a", "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "^(a)$",
        "a\na",
        "a", "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(^(a)$\n^)|b",
        "a\na\nb",
        "a\n", "a\n", "b"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "\\^",
        "^",
        "^"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "^\\^",
        "^^",
        "^"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "\\(a(ab)",
        "aab(aab (aab",
        "(aab", "(aab"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[a]",
        "aaa",
        "a", "a", "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[^a]",
        "abcaa",
        "b", "c"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[^a-z]",
        "abc123",
        "1", "2", "3"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[^a-z]*",
        "abc123ая",
        "", "", "", "123ая", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[a-z]*|[1-9]*",
        "abc123ая",
        "abc", "", "", "", "", "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[1-9]*|[a-z]*",
        "abc123ая",
        "", "", "", "123", "", "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[1-9]+|[a-z]*",
        "abc123ая",
        "abc", "123", "", "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[1-9]+|[a-z]+",
        "abc123ая",
        "abc", "123"
    );
    fail |= t.testAndLog<RegexLexer>();

    return fail;
}

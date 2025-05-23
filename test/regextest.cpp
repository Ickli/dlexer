#include <dlexer/regex.hpp>
#include "common.hpp"

using namespace dlexer;

int testGroups() {
    RegexLexer l{"([a-z]+)|([0-9]+)"};
    const std::string str = "abc 123 a1";

    std::vector<std::vector<RegexData::Group>> groups = {
        {{0, 3}, {-1, -1}},
        {{-1, -1}, {4, 7}},
        {{8, 9}, {-1, -1}},
        {{-1, -1}, {9, 10}}
    };
    std::vector<std::string> matches = {
        "abc", "123", "a", "1"
    };

    RegexData data(str);
    
    std::string out;
    for(int i = 0; i < matches.size(); ++i) {
        if(!l.getToken(out, data) || out != matches[i]) { 
            std::cerr << "mismatch, desired = " << matches[i] << "\nres = " << out << '\n';
            return 1;

        }
        auto gs = groups[i];

        for(int j = 0; j < 2; ++j) {
            auto g = gs[j];
            auto gdata = data.groups[j];
            // if(g.start == -1) { continue; }
            if(g.start != gdata.start || g.end != gdata.end) {
                std::cerr << "At match = \"" << matches[i] << "\":\n";
                std::cerr << "group mismatch, desired = "
                    "(" << g.start << ", " << g.end << ")\n"
                    << "res = (" << gdata.start << ", " << gdata.end << ")\n";
                return 1;
            }
        }
    }

    return 0;
}

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
        "a?",
        "a ba baa",
        "a", "", "", "a", "", "", "a", "a", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "ba?",
        "a ba baa",
        "ba", "ba"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "ba?",
        "a ba baa",
        "ba", "ba"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(ab)?",
        "ab ba abbaa",
        "ab", "", "", "", "", "ab", "", "", "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(ab)??",
        "aba",
        "", "", "", ""
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(ab)*?a",
        "aba",
        "a", "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(ab)+?a",
        "aba",
        "aba"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "(ab)+?a",
        "aba ababa",
        "aba", "aba"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[a-y]*?z",
        "aba ababa"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[a-y]*?z",
        "aba ababaz",
        "ababaz"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[a-y]*?z",
        "abaz ababaz",
        "abaz", "ababaz"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[а-ю]*?я",
        "абв абвабв"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "[а-ю]*?я",
        "абвя абвабвя",
        "абвя", "абвабвя"
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

    t = LexerTestCase::create(
        "//[a-z]*$",
        "asdadasda //",
        "//"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "//[a-z]*$",
        "asdadasda //abc",
        "//abc"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "//[a-z]*$",
        "asdadasda //abc\nasd//def\n//ghi",
        "//abc", "//def", "//ghi"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "([a-z])",
        "a a",
        "a", "a"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "((a|b|c))",
        "abc",
        "a", "b", "c"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "([a-z]+)",
        "abc ",
        "abc"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "([a-z]+)|([1-9]+)",
        "abc 123 a1",
        "abc", "123", "a", "1"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "([а-я]+)|([a-z]+)|([0-9]+)",
        "abc ая 123 a1",
        "abc", "ая", "123", "a", "1"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "([а-я]+)$|^d",
        "d d ааа ббб",
        "d", "ббб"
    );
    fail |= t.testAndLog<RegexLexer>();

    t = LexerTestCase::create(
        "a",
        ""
    );
    fail |= t.testAndLog<RegexLexer>();

    fail |= testGroups();

    return fail;
}

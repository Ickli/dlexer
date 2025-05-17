#include <dlexer/typed.hpp>
#include <dlexer/common.hpp>
#include <dlexer/basic.hpp>
#include <sstream>
#include <cctype>

namespace dlexer {

const char patLexerPat[] = "\\\\>\"! ^\t^\n^";
static const BasicLexer PatLexer(patLexerPat);

std::vector<TypedLexer::NameContentPair> pairsFromPattern(const std::string& pat) {
    std::vector<TypedLexer::NameContentPair> pairs;
    char unit[4];
    char includePrevMode = 0;

    std::string curpart;
    std::stringstream patstr(pat);
    bool quoted = false;

    pairs.push_back({});

    while(PatLexer.getToken(curpart, patstr, unit, includePrevMode)) {
        if(quoted) {
            if(curpart[0] == '\\') {
                pairs.back().content.append(curpart.data() + 1);
            } else if(curpart[0] != '"') {
                pairs.back().content.append(curpart.data());
            } else {
                quoted = false;
                pairs.push_back({});
            }
        } else {
            if(curpart[0] == '\\') {
                pairs.back().name.append(curpart.data() + 1);
            } else if(std::isspace(curpart[0])) {
                continue;
            } else if(curpart[0] != '"') {
                pairs.back().name.append(curpart.data());
            } else {
                quoted = true;
            }
        }
    }
    pairs.pop_back();

    for(int i = 0; i < pairs.size() && 0; ++i) {
        std::cerr << pairs[i].name << " =\"" << pairs[i].content << "\"\n";
    }
    return pairs;
}

TypedLexer::TypedLexer(std::vector<NameContentPair>&& types): types(types) {}

TypedLexer::TypedLexer(const std::string& pat): TypedLexer(pairsFromPattern(pat)) {}

void TypedLexer::reprogram(const std::string& pat) {
    types = pairsFromPattern(pat);
    endCurTokenList();
}

bool TypedLexer::getToken(std::string& out, std::istream& in) {
    return getToken(out, in, this->data);
}

bool TypedLexer::getToken(std::string& out, std::istream& in, Data& data) const {
    if(in.eof()) {
        return false;
    }

    out.clear();

    if(data.toIncludePrev) {
        out.append(data.unit, unitLength(data.unit[0]));
        data.toIncludePrev = false;
        data.outType = data.curType;
    }

    while(true) {
        const int ulen = extractUnit(data.unit, in);

        if(in.eof()) {
            return out.length() != 0;
        }

        data.curType = -1;
        for(int i = 0; i < types.size(); ++i) {
            const std::string& cnt = types[i].content;
            const int uind = findUnit(cnt.data(), cnt.size(), data.unit, ulen);
            if(uind == -1) { continue; }

            data.curType = i;
            break;
        }
        if(data.curType == -1 || data.outType != data.curType) {
            data.toIncludePrev = (data.curType != -1);
            if(out.length() == 0) { continue; }
            return true;
        }
        out.append(data.unit, ulen);
    }
}

void TypedLexer::endCurTokenList() {
    data = {0};
}

} // namespace dlexer

#include <dlexer/basic.hpp>
#include <dlexer/common.hpp>

#define FALLTHROUGH

using namespace dlexer;

BasicLexer::BasicLexer(const std::string& pat) {
    reprogram(pat);
}

BasicLexer::BasicLexer(std::vector<char>&& bounds, std::vector<IncludeType>&& incType): bounds(std::move(bounds)), incType(std::move(incType)) {}

void BasicLexer::reprogram(const std::string& pat) {
    bool toEscape = false;
    int ulen = 0;

    bounds.clear();
    incType.clear();
    endCurTokenList();

    for(int i = 0; i < pat.size(); i += ulen) {
        ulen = unitLength(pat[i]);
        if(ulen == 1) {
            if(pat[i] == '\\') {
                if(!toEscape) {
                    toEscape = true;
                    continue;
                }
            } else if(!toEscape && pat[i] == '<') {
                incType.back() = BasicLexer::LEFT_INCLUDE;
                continue;
            } else if(!toEscape && pat[i] == '>') {
                incType.back() = BasicLexer::RIGHT_INCLUDE;
                continue;
            } else if(!toEscape && pat[i] == '^') {
                incType.back() = BasicLexer::STANDALONE;
                continue;
            } else if(!toEscape && pat[i] == '!') {
                incType.back() = BasicLexer::WEAK_STANDALONE;
                continue;
            }
        } 
        for(int uoff = 0; uoff < ulen; ++uoff) {
            bounds.push_back(pat[i + uoff]);
        }
        incType.push_back(BasicLexer::NO_INCLUDE);
        toEscape = false;
    }
}

void BasicLexer::endCurTokenList() {
    includePrevMode = NO_INCLUDE;
}

bool BasicLexer::getToken(std::string& out, std::istream& in) {
    return getToken(out, in, this->unit, this->includePrevMode);
}

bool BasicLexer::getToken(std::string& out, std::istream& in, char* unit, char& includePrevMode) const {
    if(in.eof()) {
        return false;
    } 

    out.clear();

    int justIncludedMode = NO_INCLUDE;
    while(true) {
        justIncludedMode = includePrevMode;
        if(includePrevMode == RIGHT_INCLUDE) {
            out.append(unit, unitLength(unit[0]));
            includePrevMode = NO_INCLUDE;
        } else if(includePrevMode == STANDALONE || includePrevMode == WEAK_STANDALONE) {
            out.append(unit, unitLength(unit[0]));
            includePrevMode = NO_INCLUDE;
            return true;
        }

        const int ulen = extractUnit(unit, in);

        if(in.eof()) {
            return out.length() != 0;
        }

        const int uind = findUnit(bounds.data(), bounds.size(), unit, ulen);
        if(uind == -1) {
            out.append(unit, ulen);
        } else switch(incType[uind]) {
            case NO_INCLUDE: {
                if(out.length() == 0) {
                    continue;
                }
                return true;
            }
            case LEFT_INCLUDE: out.append(unit, ulen); return true;
            case RIGHT_INCLUDE: FALLTHROUGH
            case STANDALONE: {
                includePrevMode = incType[uind];
                if(out.length() == 0) {
                    continue;
                }
                return true;
            }
            case WEAK_STANDALONE: {
                includePrevMode = WEAK_STANDALONE;
                if(out.length() == 0 || justIncludedMode == RIGHT_INCLUDE) {
                    continue;
                }
                return true;
            }
        } // switch
    } // while true
}

#ifndef DLEXER_BASIC_H_
#define DLEXER_BASIC_H_
#include <vector>
#include <string>
#include <iostream>

namespace dlexer {

struct BasicLexer {
    enum IncludeType: char {
        NO_INCLUDE,
        LEFT_INCLUDE,
        RIGHT_INCLUDE,
        STANDALONE,
        WEAK_STANDALONE
    };

    std::vector<char> bounds;
    std::vector<IncludeType> incType;
    char unit[4] = {0};
    char includePrevMode = NO_INCLUDE;

    BasicLexer(const std::string& pat);
    BasicLexer(std::vector<char>&& bounds, std::vector<IncludeType>&& incType);
    
    void reprogram(const std::string& pat);
    void endCurTokenList();
    bool getToken(std::string& out, std::istream& in);
    bool getToken(std::string& out, std::istream& in, char* unit, char& includePrevMode) const;

    void writeAsCppProgram(std::ofstream& out) const; // TODO
};

} // namespace dlexer
#endif // DLEXER_BASIC_H_

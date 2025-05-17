#ifndef DLEXER_TYPED_H
#define DLEXER_TYPED_H
#include <string>
#include <vector>

namespace dlexer {

struct TypedLexer {

    struct NameContentPair {
        std::string name;
        std::string content;
    };
    std::vector<NameContentPair> types;

    struct Data {
        char unit[4];
        int outType;
        int curType;
        bool toIncludePrev;
    } data = {0};

    TypedLexer(std::vector<NameContentPair>&& types);
    TypedLexer(const std::string& pat);
    void reprogram(const std::string& pat);
    bool getToken(std::string& out, std::istream& in);
    bool getToken(std::string& out, std::istream& in, Data& data) const;
    void endCurTokenList();
    void writeAsCppProgram(std::ofstream& out) const;
};

} // namespace dlexer
#endif // DLEXER_TYPED_H

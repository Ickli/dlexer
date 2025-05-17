#include <sstream>
#include <vector>
#include <string>
#include <iostream>

struct LexerTestCase {
    std::string pat;
    std::string str;
    std::vector<std::string> desired;
    std::vector<std::string> res;
    std::string err;

    template<typename... DesiredArgs>
    static LexerTestCase create(std::string&& pat, std::string&& str, DesiredArgs... args) {
        LexerTestCase t{};
        t.pat = std::move(pat);
        t.str = std::move(str);
        t.desired = std::vector<std::string>{args...};
        return t;
    }

    template<typename LexerT>
    int testAndLog() {
        if(!test<LexerT>()) {
            std::cerr << "FAIL AT PATTERN: \"" << pat << "\", STRING: \"" << str << "\"\n";
            std::cerr << err;
            return 1;
        }
        return 0;
    }

    template<typename LexerT>
    bool test() {
        tokenize<LexerT>();

        std::stringstream errstr;
        if(desired.size() != res.size()) {
            errstr << "Desired size (" << desired.size() << ")"
                << " != "
                << "Result size (" << res.size() << ")\n";
            outDesiredAndRes(errstr);
            err = errstr.str();
            return false;
        }

        for(int i = 0; i < desired.size(); ++i) {
            if(desired[i] != res[i]) {
                errstr << i << "'th elements don't match: "
                    << "desired = \"" << desired[i] << "\", res = \"" <<
                    res[i] << "\"\n";
                outDesiredAndRes(errstr);
                err = errstr.str();
                return false;
            }
        }
        return true;
    }

    template<typename LexerT>
    void tokenize() {
        auto in = std::stringstream(this->str);
        LexerT l = LexerT(this->pat);
        this->res.clear();
        this->res.reserve(this->desired.size());

        std::string cur = "";
        while(l.getToken(cur, in)) {
            res.push_back(cur);
        }
        /*
        if(cur.length() != 0) {
            res.push_back(cur);
        }
        */
    }

    void outDesiredAndRes(std::ostream& out) {
        out << "[";
        for(int i = 0; i < desired.size(); ++i) {
            out << '\"' << desired[i] << "\", ";
        }
        out << "] = desired\n";

        out << "[";
        for(int i = 0; i < res.size(); ++i) {
            out << '\"' << res[i] << "\", ";
        }
        out << "] = res\n";
    }
};

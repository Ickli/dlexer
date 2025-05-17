#include <dlexer/common.hpp>
#include <cstring>

namespace dlexer {

int findUnit(const char* arr, size_t arrLen, const char* unit, size_t ulen) {
    int unitInd = 0;
    for(int i = 0; i + ulen <= arrLen; i += ulen, ++unitInd) {
        if(memcmp(arr + i, unit, ulen) == 0) {
            return unitInd;
        }
    }
    return -1;
}

int extractUnit(char* dst, std::istream& src) {
    int len = 0;
    const char& first = dst[0] = src.get();

    if(src.eof()) {
        return 0;
    }

    len = unitLength(first);
    for(int i = 1; i < len; ++i) {
        // NOTE: assumes getting the rest of unit is guaranteed
        // TODO: make more efficient, cuz every call to get creates new
        //      sentry object
        dst[i] = src.get();
    }
    return len;
}

int extractUnitStr(char* dst, const char* src) {
    int len = unitLength(src[0]);
    
    for(int i = 0; i < len; ++i) {
        dst[i] = src[i];
    }
    return len;
}

int unitLength(char first) {
    int ind = 0;
    while(first & (1 << (8*sizeof(char) - 1 - ind))) { 
        ind++;
    }

    return ind + (ind == 0);
}

int unitLengthLast(const char* last) {
    int off = 0;
    static const int check = (2 << (8*sizeof(char) - 2));
    while((*last & (2 << (8*sizeof(char) - 2))) == check) { last--; off++; }
    return off + 1;
}
} // namespace dlexer

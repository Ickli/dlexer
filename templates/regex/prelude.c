#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define GETTOK_ARGS(state, thisAt, thisStartPos, thisPos) getToken_(str, strLen, startPos, pos, at, unit, ulen, groups, state, thisAt, thisStartPos, thisPos)

typedef enum {
    LINE_AT_START,
    LINE_AT_MID,
    LINE_AT_END,
    LINE_AT_EOF,
    LINE_AT_PAST_EOF,
} LineAt;

int unitLength(char first) {
    int ind = 0;
    while(first & (1 << (8*sizeof(char) - 1 - ind))) { 
        ind++;
    }

    return ind + (ind == 0);
}

int extractUnitStr(char* dst, const char* src) {
    int len = unitLength(src[0]);
    
    for(int i = 0; i < len; ++i) {
        dst[i] = src[i];
    }
    return len;
}

int tryFetch(
    const char* str, 
    const int* strLen, 
    char* unit, 
    int* ulen,
    int* pos,
    LineAt* at
) {
    if(*pos < *strLen) {
        *ulen = extractUnitStr(unit, str + *pos);
        *pos += *ulen;

        if(*pos == *strLen) { *at = LINE_AT_EOF; } 
        else if(pos == 0) { *at = LINE_AT_START; }
        else if(*ulen == 1 && unit[0] == '\n') { *at = LINE_AT_END; }
        else if(unitLength(str[*pos]) == 1 && str[*pos] == '\n') { *at = LINE_AT_END; }
        else { *at = LINE_AT_MID; }

    } else {
        ulen = 0;
    }
    return ulen != 0;
}

int getToken_(
    const char* str, 
    const int* strLen, 
    int* startPos,
    int* pos,
    LineAt* at,
    char* unit, 
    int* ulen,
    int* groups,
    int state,
    LineAt thisAt,
    int thisStartPos,
    int thisPos
) {
    switch(state) {

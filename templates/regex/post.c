    default: return 0;
    }
}

int getToken(
    const char* str, const int* strLen, int* startPos,
    int* pos, LineAt* at, char* unit, int* ulen,
    int* groups, int groupsCount, int state, 
    LineAt thisAt, int thisStartPos, 
    int thisPos
) {
    for(int i = 0; i < groupsCount; ++i) {
        groups[i] = -1;
    }
    return getToken_(str, strLen, startPos, pos, 
        at, unit, ulen, groups, state, thisAt, thisStartPos, thisPos);
}


#define GROUPSIZE 6
int main() {
    const char str[] = "aa 123 abc вzбя";
    const int strLen = sizeof(str) - 1;
    int groups[GROUPSIZE];
    char unit[4];
    int ulen;
    int startPos = 0;
    int pos = 0;
    LineAt at = LINE_AT_START;
    while(getToken(str, &strLen, &startPos, &pos, &at, unit, 
        &ulen, groups, GROUPSIZE, 0, LINE_AT_START, 0, 0)) {
        printf("%.*s\n", pos - startPos, str + startPos);

        printf("GROUPS:\n");
        for(int i = 0; i < sizeof(groups)/sizeof(groups[0]); i += 2) {
            printf("{%d, %d}, ", groups[i], groups[i+1]);
        }
        printf("\n");
    }
}

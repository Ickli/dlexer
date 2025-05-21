    default: return 0;
    }
}

int main() {
    const char str[] = "aa aaa";
    const int strLen = sizeof(str) - 1;
    char unit[4];
    int ulen;
    int startPos = 0;
    int pos = 0;
    LineAt at = LINE_AT_START;
    while(getToken(str, &strLen, &startPos, &pos, &at, unit, 
        &ulen, 0, LINE_AT_START, 0, 0)) {
        printf("%.*s\n", pos - startPos, str + startPos);
    }
}

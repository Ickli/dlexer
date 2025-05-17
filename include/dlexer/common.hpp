#include <string>
#include <iostream>

namespace dlexer {

// returns number of units before the unit in the array
// if not found, returns -1
int findUnit(const char* arr, size_t arrLen, const char* unit, size_t ulen);
int extractUnit(char* dst, std::istream& src);
int extractUnitStr(char* dst, const char* src);
int unitLength(char first);
int unitLengthLast(const char* last);

} // namespace dlexer

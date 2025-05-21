cmake_minimum_required(VERSION 3.14)

# declared outside: INCLUDE_DIRS, dlexer

add_executable(basictest basictest.cpp)
target_include_directories(basictest PRIVATE "${INCLUDE_DIRS}")
target_link_libraries(basictest PRIVATE dlexer)
add_test(NAME TestBasicLexer COMMAND basictest)

add_executable(typedtest typedtest.cpp)
target_include_directories(typedtest PRIVATE "${INCLUDE_DIRS}")
target_link_libraries(typedtest PRIVATE dlexer)
add_test(NAME TestTypedLexer COMMAND typedtest)

add_executable(regextest regextest.cpp)
target_include_directories(regextest PRIVATE "${INCLUDE_DIRS}")
target_link_libraries(regextest PRIVATE dlexer)
add_test(NAME TestRegexLexer COMMAND regextest)

add_executable(testmain testmain.cpp)
target_include_directories(testmain PRIVATE "${INCLUDE_DIRS}")
target_link_libraries(testmain PRIVATE dlexer)

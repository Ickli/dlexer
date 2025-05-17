cmake_minimum_required(VERSION 3.14)

project(dlexer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(BUILD_TESTING ON)

set(SOURCE_FILES
    common.cpp
    basic.cpp
    typed.cpp
    regex.cpp

    # strem_structures/strem_vector.c
)
set(INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

add_library(dlexer "${SOURCE_FILES}")
target_include_directories(dlexer PUBLIC ${INCLUDE_DIRS})

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

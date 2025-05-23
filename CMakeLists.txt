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
)

set(TEMPLATES
    "regex/prelude.c"
    "regex/post.c"
)

set(INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

add_library(dlexer "${SOURCE_FILES}")
target_include_directories(dlexer PUBLIC ${INCLUDE_DIRS})



set(BIN_TEMPLATES_PATH "${CMAKE_CURRENT_BINARY_DIR}/templates")
set(SRC_TEMPLATES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/templates")

set(TEMPLATES_ABS)

foreach(tmp IN LISTS TEMPLATES)
    add_custom_command(
        OUTPUT "${BIN_TEMPLATES_PATH}/${tmp}"
        COMMAND ${CMAKE_COMMAND} -E copy "${SRC_TEMPLATES_PATH}/${tmp}" "${BIN_TEMPLATES_PATH}/${tmp}"
        DEPENDS "${SRC_TEMPLATES_PATH}/${tmp}"
    )
    # make_directory("${BIN_TEMPLATES_PATH}/${tmp}")
    list(APPEND TEMPLATES_ABS "${BIN_TEMPLATES_PATH}/${tmp}")
endforeach()

add_custom_target(templates_target ALL DEPENDS ${TEMPLATES_ABS})
# add_dependencies(dlexer templates_target)
message(${TEMPLATES_ABS})

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

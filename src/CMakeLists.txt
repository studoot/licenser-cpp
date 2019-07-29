cmake_minimum_required(VERSION 3.14)

project(licenser-cpp)

set(CMAKE_CXX_STANDARD 17)

find_package(fmt CONFIG REQUIRED)

add_library(peglib INTERFACE)
target_include_directories(peglib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../externals/peglib)

add_executable(test test.cpp license.peg)
target_include_directories(test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/license.peg.h
    COMMAND xxd ARGS -i license.peg ${CMAKE_CURRENT_BINARY_DIR}/license.peg.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/license.peg)
target_link_libraries(test PRIVATE fmt::fmt-header-only peglib)
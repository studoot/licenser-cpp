cmake_minimum_required(VERSION 3.14)

project(licenser-cpp)

set(CMAKE_CXX_STANDARD 17)

find_package(fmt CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
add_library(peglib INTERFACE)
target_include_directories(peglib INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../externals/peglib)

add_library(NamedType INTERFACE)
target_include_directories(NamedType INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../externals/named-type)

add_executable(license test.cpp license.cpp license-parser.cpp license.peg)

file(READ ${CMAKE_CURRENT_LIST_DIR}/license.peg LICENSE_PEG)
configure_file(license.peg.hpp.in license.peg.hpp)

target_compile_definitions(license PRIVATE DOCTEST_CONFIG_DISABLE)
target_include_directories(license PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(license PRIVATE fmt::fmt peglib NamedType doctest::doctest)


add_executable(test test-main.cpp license.cpp license-parser.cpp license.peg)

file(READ ${CMAKE_CURRENT_LIST_DIR}/license.peg LICENSE_PEG)
configure_file(license.peg.hpp.in license.peg.hpp)
target_include_directories(test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(test PRIVATE fmt::fmt peglib NamedType doctest::doctest)
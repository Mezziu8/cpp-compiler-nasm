cmake_minimum_required(VERSION 3.16)
project(CompilerForNASMCpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(FLEX REQUIRED)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp)

flex_target(scanner ${SRC_DIR}/scanner.l ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp)

add_executable(compiler ${SOURCES} ${FLEX_scanner_OUTPUTS})

target_include_directories(compiler PRIVATE ${INC_DIR} ${CMAKE_CURRENT_BINARY_DIR})



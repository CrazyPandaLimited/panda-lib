CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(Panda-Lib VERSION 1.1.5 LANGUAGES CXX)
enable_testing()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_TYPE STATIC)

file(GLOB_RECURSE libPandaSource RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cc")
add_library(panda-lib ${LIB_TYPE} ${libPandaSource})
target_include_directories(panda-lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
    )

#tests
file(GLOB_RECURSE testSource RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "tests/*.cc")
list(FILTER testSource EXCLUDE REGEX "tests/main.cc")

add_library(panda-lib-tests  STATIC ${testSource})
target_link_libraries(panda-lib-tests PUBLIC panda-lib)

find_package(Catch2 QUIET)
target_link_libraries(panda-lib-tests PUBLIC Catch2::Catch2)

get_target_property(Catch2_INCLUDE_DIR Catch2::Catch2 INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(panda-lib-tests PUBLIC ${Catch2_INCLUDE_DIR}/catch2) #tmp fix because we use Catch2 wrong, it should be include <catch/catch.hpp>, not <catch.hpp>

#ctest
add_executable(panda-lib-test-runner ${testSource} "tests/main.cc")
#target_compile_definitions(panda-lib-test-runner PRIVATE CATCH_CONFIG_MAIN)
target_link_libraries(panda-lib-test-runner panda-lib-tests)
add_test(test-all panda-lib-test-runner)

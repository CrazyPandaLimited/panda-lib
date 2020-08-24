CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(Panda-Lib VERSION 1.1.5 LANGUAGES CXX)
enable_testing()

set(LIB_TYPE STATIC)

file(GLOB_RECURSE libPandaSource RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.cc")
add_library(panda-lib ${LIB_TYPE} ${libPandaSource})
target_include_directories(panda-lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(panda-lib PUBLIC cxx_std_14)

########################tests#######################################
file(GLOB_RECURSE testSource RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "t/*.cc")
list(FILTER testSource EXCLUDE REGEX "t/main.cc")

add_library(panda-lib-tests  STATIC ${testSource})
target_link_libraries(panda-lib-tests PUBLIC panda-lib)

find_package(Catch2 QUIET)
if (Catch2_FOUND)
    target_link_libraries(panda-lib-tests PUBLIC Catch2::Catch2)

    get_target_property(Catch2_INCLUDE_DIR Catch2::Catch2 INTERFACE_INCLUDE_DIRECTORIES)
    target_include_directories(panda-lib-tests PUBLIC ${Catch2_INCLUDE_DIR}/catch2) #tmp fix because we use Catch2 wrong, it should be include <catch/catch.hpp>, not <catch.hpp>
endif()

########################ctests######################################
#executable should be MyTest to pass test esception.cc that checks "library" name
add_executable(MyTest ${testSource} "t/main.cc")
target_link_libraries(MyTest panda-lib-tests)
add_test(test-all MyTest)

########################install#####################################
install(DIRECTORY src/ DESTINATION include
    FILES_MATCHING PATTERN "*.h")
install(TARGETS panda-lib EXPORT panda-libTargets
    ARCHIVE DESTINATION lib
)

install(EXPORT panda-libTargets
    FILE panda-libConfig.cmake
    NAMESPACE panda::
    DESTINATION lib/cmake/panda-lib
)

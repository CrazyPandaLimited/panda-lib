cmake_minimum_required(VERSION 3.12)
project(libpanda VERSION 1.1.5)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "src/*.h")
file(GLOB_RECURSE sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "src/*.cc")

include(CheckCSourceCompiles)

check_c_source_compiles("
    #include <stdint.h>
    int main () { return __builtin_bswap64(12345) == 0; }
" HAVE_BSWAP)

if(NOT LIBPANDA_HAVE_BSWAP)
    check_c_source_compiles("
        #include <stdlib.h>
        int main () { return _byteswap_uint64(12345) == 0; }
    " HAVE_BYTESWAP)
endif()

configure_file(src/panda/endian_config.h.in src/panda/endian_config.h)

include_directories(src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

add_library(libpanda SHARED ${sources})

add_library(libpanda-static EXCLUDE_FROM_ALL STATIC ${sources})
set_target_properties(libpanda-static PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME "libpanda")

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
    #include <stdlib.h>
    int main () { return __builtin_bswap64(12345) == 0; }
" LIBPANDA_HAVE_BSWAP)

if(NOT LIBPANDA_HAVE_BSWAP)
    check_c_source_compiles("
        #include <stdint.h>
        #include <stdlib.h>
        int main () { return _byteswap_uint64(12345) == 0; }
    " LIBPANDA_HAVE_BYTESWAP)
endif()

add_library(libpanda SHARED ${sources})
target_include_directories(libpanda PRIVATE src)

add_library(libpanda-static STATIC ${sources})
target_include_directories(libpanda-static PRIVATE src)
set_target_properties(libpanda-static PROPERTIES LINKER_LANGUAGE CXX OUTPUT_NAME "libpanda")

include(FetchContent)

set(Catch2_repository https://github.com/catchorg/Catch2.git)
set(Catch2_repository_tag devel)

if (${PANDALIB_TESTS})
    set(deps Catch2)
endif()

foreach(dep ${deps})
    if (NOT ${dep}_POPULATED)
        if (NOT DEFINED ${${dep}_repository_tag})
            set (${${dep}_repository_tag} master)
        endif()
        FetchContent_Declare(${dep}
            GIT_REPOSITORY ${${dep}_repository}
            GIT_TAG ${${dep}_repository_tag}
        )
        FetchContent_MakeAvailable(${dep})
    endif ()
endforeach()
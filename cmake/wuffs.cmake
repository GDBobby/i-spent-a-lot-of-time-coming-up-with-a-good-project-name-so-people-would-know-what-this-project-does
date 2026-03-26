
if(NOT TARGET wuffs::wuffs)
    FetchContent_Declare(
        wuffs
        GIT_REPOSITORY https://github.com/google/wuffs
        GIT_TAG        b914563
        GIT_SHALLOW    ON
        GIT_PROGRESS TRUE
        EXCLUDE_FROM_ALL
    )

    FetchContent_MakeAvailable(wuffs)

    add_library(wuffs_lib INTERFACE)
    target_include_directories(wuffs_lib INTERFACE "${wuffs_SOURCE_DIR}/release/c")

    add_library(wuffs::wuffs ALIAS wuffs_lib)
endif()
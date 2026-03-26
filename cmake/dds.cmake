include(FetchContent)

if(NOT TARGET dds_image::dds_image)
    FetchContent_Declare(
        dds_image
        GIT_REPOSITORY https://github.com/spnda/dds_image
        GIT_TAG        0afae97
        GIT_SHALLOW    ON
        GIT_PROGRESS TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(dds_image)

    add_library(dds_image_lib INTERFACE)
    target_include_directories(dds_image_lib INTERFACE "${dds_image_SOURCE_DIR}/include")

    add_library(dds_image::dds_image ALIAS dds_image_lib)
endif()
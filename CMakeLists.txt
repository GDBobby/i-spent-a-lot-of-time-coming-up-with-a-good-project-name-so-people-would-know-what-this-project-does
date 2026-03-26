

#this is for usage as a library

cmake_minimum_required(VERSION 3.24)
project(ImageProcessor VERSION 1.0.0)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#each depended library
include(FetchContent)
set(FETCHCONTENT_FULLY_DISCONNECTED OFF)
include(cmake/dds.cmake)
include(cmake/wuffs.cmake)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE HEADER_FILES ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dds_image::dds_image
    wuffs::wuffs
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_subdirectory(example)
endif()

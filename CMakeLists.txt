cmake_minimum_required(VERSION 3.22)
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)

project(
    vkInit
    VERSION 1.0
    LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


add_definitions(-DRESPATH=\"${CMAKE_SOURCE_DIR}/res/\")
add_subdirectory(res)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(vendor EXCLUDE_FROM_ALL)
add_subdirectory(src)

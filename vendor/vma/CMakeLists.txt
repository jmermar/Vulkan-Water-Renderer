project (vma VERSION 1.1.1 LANGUAGES CXX)

find_package(Vulkan REQUIRED)

add_library(${PROJECT_NAME} STATIC)
target_include_directories(vma PUBLIC include ${Vulkan_INCLUDE_DIRS})

target_sources(vma PRIVATE "src/imp.cpp")

target_link_libraries(vma PUBLIC
    ${Vulkan_LIBRARIES}
)
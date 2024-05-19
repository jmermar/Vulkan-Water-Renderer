project (imgui)

file(GLOB_RECURSE SRC
     "*.cpp"
)


add_library(imgui STATIC)
target_include_directories(imgui PUBLIC include )

find_package(Vulkan REQUIRED)

target_sources(imgui PRIVATE ${SRC} PRIVATE)

target_link_libraries(imgui PRIVATE
    ${Vulkan_LIBRARIES}
    SDL3::SDL3
)
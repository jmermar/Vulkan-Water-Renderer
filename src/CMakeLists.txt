file(GLOB_RECURSE SRC
     "*.cpp" "foundation/*.cpp"  "val/*.cpp"
)

add_executable (${PROJECT_NAME} ${SRC})

add_dependencies(${PROJECT_NAME} Shaders)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

if(Win32)
    target_compile_options(${PROJECT_NAME} PRIVATE "/warnaserror")
elseif(UNIX)
    target_compile_options(${PROJECT_NAME} PRIVATE "-Werror")
endif(Win32)

target_compile_definitions(${PROJECT_NAME} PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")


find_package(Vulkan REQUIRED)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    ${Vulkan_LIBRARIES}
    SDL3::SDL3
    vk-bootstrap::vk-bootstrap
    vma
    stb_image
    imgui
)

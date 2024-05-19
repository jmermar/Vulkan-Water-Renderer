set(SDL_VIDEO TRUE)
set(LINUX TRUE)
set(SDL_VULKAN TRUE)

add_subdirectory(SDL)
add_subdirectory(vk-bootstrap)
add_subdirectory(glm)
add_subdirectory(vma)
add_subdirectory(stb-image)

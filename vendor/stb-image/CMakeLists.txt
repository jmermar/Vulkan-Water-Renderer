project (stb_image)

add_library(${PROJECT_NAME} STATIC)
target_include_directories(stb_image PUBLIC include )

target_sources(stb_image PRIVATE "src/stb_image.c")

#include "file.hpp"

#include <stb_image.h>

#include <cstring>
#include <fstream>

namespace file {
std::vector<uint8_t> readBinary(const std::string& path) {
    std::ifstream file(std::string(RESPATH) + path,
                       std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot read file: " + path);
    }

    size_t fileSize = (size_t)file.tellg();

    std::vector<uint8_t> buffer(fileSize);

    file.seekg(0);

    file.read((char*)buffer.data(), fileSize);
    return buffer;
}

ImageData loadImage(const std::string& path) {
    int w, h, channels;
    std::string fullPath = std::string(RESPATH) + path;
    uint8_t* img = stbi_load(fullPath.c_str(), &w, &h, &channels, 4);
    ImageData ret;
    ret.size.w = w;
    ret.size.h = h;
    ret.data.resize(w * h * 4);
    memcpy(ret.data.data(), img, w * h * 4);
    stbi_image_free(img);
    return ret;
}
}  // namespace file

#include "helpers.hpp"

namespace val {
namespace helpers {
size_t getTextureSizeFromSizeAndFormat(const Size s, TextureFormat format) {
    int pixelSize = 4;
    if (format == TextureFormat::RGBA16) {
        pixelSize = 8;
    }

    return s.w * s.h * pixelSize;
}
}  // namespace helpers
}  // namespace val

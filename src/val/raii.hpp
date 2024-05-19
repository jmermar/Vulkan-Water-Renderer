#pragma once
#include <vma/vk_mem_alloc.h>

#include "types.hpp"
namespace raii {
class NoCopy {
   public:
    NoCopy() = default;
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;
};

class VMA : NoCopy {
   private:
    VmaAllocator allocator{};

   public:
    void init(const VmaAllocatorCreateInfo& info) {
        vmaCreateAllocator(&info, &allocator);
    }
    void free() {
        if (allocator) {
            vmaDestroyAllocator(allocator);
            allocator = 0;
        }
    }

    VMA() = default;
    ~VMA() { free(); }

    VMA(const VmaAllocatorCreateInfo& info) : VMA() { init(info); }

    VMA(VMA&& other) {
        allocator = other.allocator;
        other.allocator = 0;
    }

    VMA& operator=(VMA&& other) {
        free();
        allocator = other.allocator;
        other.allocator = 0;
        return *this;
    }

    operator VmaAllocator() { return allocator; }
};

class Buffer : NoCopy {
   private:
    vk::Buffer buffer{};

   public:
    VmaAllocationInfo allocInfo{};
    VmaAllocator vma{};
    VmaAllocation alloc{};
    void free() {
        if (vma) {
            vmaDestroyBuffer(vma, buffer, alloc);
            vma = 0;
            buffer = vk::Buffer();
            alloc = 0;
        }
    }

    void init(VmaAllocator vma, const VkBufferCreateInfo& bufferCreateInfo,
              const VmaAllocationCreateInfo& allocCreateInfo) {
        free();
        VkBuffer b{};
        if (VK_SUCCESS == vmaCreateBuffer(vma, &bufferCreateInfo,
                                          &allocCreateInfo, &b, &alloc,
                                          &allocInfo)) {
            this->vma = vma;
            buffer = b;
        }
    }

    Buffer() = default;
    Buffer(VmaAllocator vma, const VkBufferCreateInfo& bufferCreateInfo,
           const VmaAllocationCreateInfo& allocCreateInfo)
        : Buffer() {
        init(vma, bufferCreateInfo, allocCreateInfo);
    }

    ~Buffer() { free(); }

    Buffer(Buffer&& other) {
        alloc = other.alloc;
        buffer = other.buffer;
        vma = other.vma;
        allocInfo = other.allocInfo;

        other.alloc = 0;
        other.vma = 0;
        other.buffer = vk::Buffer();
    }

    Buffer& operator=(Buffer&& other) {
        free();
        alloc = other.alloc;
        buffer = other.buffer;
        vma = other.vma;
        allocInfo = other.allocInfo;

        other.alloc = 0;
        other.vma = 0;
        other.buffer = vk::Buffer();

        return *this;
    }

    operator vk::Buffer() { return buffer; }
};

class Image : NoCopy {
   private:
    vk::Image image{};

   public:
    VmaAllocator vma{};
    VmaAllocation alloc{};
    VmaAllocationInfo allocInfo{};
    void free() {
        if (vma) {
            vmaDestroyImage(vma, image, alloc);
            vma = 0;
            image = vk::Image();
            alloc = 0;
        }
    }

    void init(VmaAllocator vma, const VkImageCreateInfo& imagecreateInfo,
              const VmaAllocationCreateInfo& allocCreateInfo) {
        free();
        VkImage i{};
        if (VK_SUCCESS == vmaCreateImage(vma, &imagecreateInfo,
                                         &allocCreateInfo, &i, &alloc,
                                         &allocInfo)) {
            this->vma = vma;
            image = i;
        }
    }

    Image() = default;
    Image(VmaAllocator vma, const VkImageCreateInfo& imageCreateInfo,
          const VmaAllocationCreateInfo& allocCreateInfo)
        : Image() {
        init(vma, imageCreateInfo, allocCreateInfo);
    }

    ~Image() { free(); }

    Image(Image&& other) {
        alloc = other.alloc;
        image = other.image;
        vma = other.vma;
        allocInfo = other.allocInfo;

        other.alloc = 0;
        other.vma = 0;
        other.image = vk::Image();
    }

    Image& operator=(Image&& other) {
        free();
        image = other.image;
        alloc = other.alloc;
        vma = other.vma;
        allocInfo = other.allocInfo;

        other.vma = 0;
        other.image = vk::Image();

        return *this;
    }

    operator vk::Image() { return image; }
};
};  // namespace raii

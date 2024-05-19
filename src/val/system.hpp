#pragma once
#include <cassert>

#include "../foundation/memory.hpp"
#include "binding.hpp"
#include "commands.hpp"
#include "gpu_resources.hpp"
#include "raii.hpp"
#include "types.hpp"

namespace val {

class PresentationProvider {
   public:
    virtual VkSurfaceKHR getSurface(VkInstance ins) = 0;
    virtual Size getSize() = 0;
};

class Engine {
    friend class PipelineBuilder;
    friend class CommandBuffer;
    friend class ComputePipelineBuilder;

   private:
    // types

    struct DeletionQueue {
        std::vector<Texture> textures;
        std::vector<StorageBuffer> buffers;
        std::vector<Mesh> meshes;
        std::vector<raii::Buffer> rawBuffers;

        void clear() {
            textures.clear();
            buffers.clear();
            meshes.clear();
            rawBuffers.clear();
        }
    };

    struct Swapchain {
        std::vector<vk::Image> images;
        std::vector<vk::raii::ImageView> imageViews;

        vk::raii::SwapchainKHR swapchain{nullptr};
    };

    struct FrameData {
        vk::raii::CommandPool pool{nullptr};
        vk::raii::CommandBuffer commandBuffer{nullptr};

        vk::raii::Semaphore swapchainSemaphore{nullptr},
            renderSemaphore{nullptr};
        vk::raii::Fence renderFence{nullptr};

        DeletionQueue deletionQueue;
    };

    // Info variables
    Size windowSize;
    EngineInitConfig initConfig;
    bool _shouldClose = false;
    uint32_t frameCounter{};
    uint32_t swapchainImageIndex = 0;
    uint32_t imageIndex = 0;
    bool shouldRegenerate = false;

    // System
    Ref<PresentationProvider> presentation;

    // Vulkan Components
    vk::raii::Context ctx;
    vk::raii::Instance instance{nullptr};
    vk::raii::Device device{nullptr};
    vk::raii::DebugUtilsMessengerEXT debug_messenger{nullptr};
    vk::raii::PhysicalDevice chosenGPU{nullptr};
    vk::raii::SurfaceKHR surface{nullptr};

    raii::VMA vma;

    vk::Queue graphicsQueue;
    vk::Queue presentQueue;

    uint32_t graphicsQueueFamily;
    uint32_t presentQueueFamily;

    vk::PhysicalDeviceProperties physicalDeviceProperties;

    Swapchain swapchain;
    FrameData frames[FRAMES_IN_FLIGHT];

    GlobalBinding bindings;

    Pool<StorageBuffer, 4096> bufferPool;
    Pool<Texture, 4096> texturePool;
    Pool<Mesh, 4096> meshPool;
    Pool<CPUBuffer, 4096> cpuBufferPool;

    DeletionQueue deletionQueue;

    void initVulkan();
    void reloadSwapchain();
    void initFrameData();

    void regenerate();

   public:
    Engine() = default;
    Engine(const EngineInitConfig& initConfig,
           Ref<PresentationProvider> presentation);

    void update();

    CommandBuffer initFrame();

    void submitFrame(Texture* backbuffer);

    Texture* createTexture(Size size, TextureFormat format,
                           TextureSampler sampling = TextureSampler::NEAREST,
                           uint32_t mipLevels = 1, VkImageUsageFlags usage = 0);
    CPUBuffer* createCpuBuffer(size_t size);
    StorageBuffer* createStorageBuffer(uint32_t size, vk::BufferUsageFlagBits usage = vk::BufferUsageFlagBits(0));

    Mesh* createMesh(size_t verticesSize, uint32_t indicesCount);

    void updateCPUBuffer(CPUBuffer* buffer, void* data, size_t size) {
        assert(buffer->size == size);
        memcpy(buffer->buffer.allocInfo.pMappedData, data, size);
    }

    void freeTexture(Texture* t) {
        deletionQueue.textures.push_back(std::move(*t));
        texturePool.destroy(t);
    }

    void destroyCpuBuffer(CPUBuffer* buffer) {
        deletionQueue.rawBuffers.push_back(std::move(buffer->buffer));
        cpuBufferPool.destroy(buffer);
    }

    void destroyStorageBuffer(StorageBuffer* buffer) {
        deletionQueue.buffers.push_back(std::move(*buffer));
        bufferPool.destroy(buffer);
    }

    void destroyMesh(Mesh* mesh) {
        deletionQueue.meshes.push_back(std::move(*mesh));
        meshPool.destroy(mesh);
    }

    vk::DescriptorSetLayout getDescriptorSetLayout() {
        return bindings.getLayout();
    }

    void waitFinishAllCommands() { device.waitIdle(); }
};
}  // namespace val

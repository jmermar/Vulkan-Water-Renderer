#include "system.hpp"

#include <VkBootstrap.h>
namespace val {
void Engine::initVulkan() {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers(true)
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .build();

    vkb::Instance vkb_inst = inst_ret.value();
    instance = vk::raii::Instance(ctx, vkb_inst.instance, nullptr);
    debug_messenger =
        vk::raii::DebugUtilsMessengerEXT(instance, vkb_inst.debug_messenger);

    auto surface = presentation->getSurface(*instance);
    this->surface = vk::raii::SurfaceKHR(instance, surface);

    vk::PhysicalDeviceVulkan13Features features = initConfig.features;
    features.dynamicRendering = true;
    features.synchronization2 = true;

    vk::PhysicalDeviceVulkan12Features features12 = initConfig.features12;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.runtimeDescriptorArray = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.shaderSampledImageArrayNonUniformIndexing = true;
    features12.descriptorBindingSampledImageUpdateAfterBind = true;
    features12.shaderUniformBufferArrayNonUniformIndexing = true;
    features12.descriptorBindingUniformBufferUpdateAfterBind = true;
    features12.shaderStorageBufferArrayNonUniformIndexing = true;
    features12.descriptorBindingStorageBufferUpdateAfterBind = true;

    vk::PhysicalDeviceFeatures features10 = initConfig.features10;
    
    vkb::PhysicalDeviceSelector selector{vkb_inst};
    vkb::PhysicalDevice physicalDevice =
        selector.set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_required_features_12(features12)
            .set_required_features(features10)
            .set_surface(surface)
            .select()
            .value();

    vkb::DeviceBuilder deviceBuilder{physicalDevice};

    vkb::Device vkbDevice = deviceBuilder.build().value();

    chosenGPU = vk::raii::PhysicalDevice(instance, physicalDevice);

    device = vk::raii::Device(chosenGPU, vkbDevice.device);

    graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();

    graphicsQueueFamily =
        vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    presentQueueFamily =
        vkbDevice.get_queue_index(vkb::QueueType::present).value();

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = *chosenGPU;
    allocatorInfo.device = *device;
    allocatorInfo.instance = *instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vma.init(allocatorInfo);

    physicalDeviceProperties = chosenGPU.getProperties();
}

void Engine::reloadSwapchain() {
    swapchain.swapchain = vk::raii::SwapchainKHR(nullptr);
    swapchain.images.clear();
    swapchain.imageViews.clear();
    auto vkbSwapchain =
        vkb::SwapchainBuilder(*chosenGPU, *device, *surface)
            .set_desired_format(VkSurfaceFormatKHR{
                .format = VK_FORMAT_B8G8R8A8_UNORM,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
            .set_desired_present_mode((VkPresentModeKHR)initConfig.presentation)
            .set_desired_extent(windowSize.w, windowSize.h)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    windowSize.w = vkbSwapchain.extent.width;
    windowSize.h = vkbSwapchain.extent.height;

    swapchain.swapchain =
        vk::raii::SwapchainKHR(device, vkbSwapchain.swapchain);

    swapchain.images.reserve(vkbSwapchain.image_count);
    swapchain.imageViews.reserve(vkbSwapchain.image_count);

    auto images = vkbSwapchain.get_images().value();
    auto imageViews = vkbSwapchain.get_image_views().value();

    for (size_t i = 0; i < vkbSwapchain.image_count; i++) {
        swapchain.images.push_back(images[i]);
        swapchain.imageViews.push_back(
            vk::raii::ImageView(device, imageViews[i]));
    }
}

void Engine::initFrameData() {
    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        auto& frame = frames[i];

        vk::CommandPoolCreateInfo commandPoolInfo{};

        commandPoolInfo.flags =
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        commandPoolInfo.queueFamilyIndex = graphicsQueueFamily;

        frame.pool = vk::raii::CommandPool(device, commandPoolInfo);

        vk::CommandBufferAllocateInfo cmdAllocInfo;
        cmdAllocInfo.commandPool = *frame.pool;
        cmdAllocInfo.commandBufferCount = 1;
        cmdAllocInfo.level = vk::CommandBufferLevel::ePrimary;

        frame.commandBuffer =
            std::move(device.allocateCommandBuffers(cmdAllocInfo)[0]);

        vk::FenceCreateInfo fenceCreate;
        fenceCreate.flags = vk::FenceCreateFlagBits::eSignaled;
        frame.renderFence = device.createFence(fenceCreate);

        vk::SemaphoreCreateInfo semaphoreCreate;
        frame.swapchainSemaphore = device.createSemaphore(semaphoreCreate);
        frame.renderSemaphore = device.createSemaphore(semaphoreCreate);
    }
}

void Engine::regenerate() {
    windowSize = presentation->getSize();

    reloadSwapchain();
}

Engine::Engine(const EngineInitConfig& initConfig,
               Ref<PresentationProvider> presentation)
    : presentation(presentation) {
    this->initConfig = initConfig;
    initVulkan();
    reloadSwapchain();
    initFrameData();
    bindings.init(device, physicalDeviceProperties);

    if (initConfig.useImGUI) {
        initImgui();
    }
}

void Engine::initImgui() {
     vk::DescriptorPoolSize pool_sizes[] = {
        {vk::DescriptorType::eSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eSampledImage, 1000},
        {vk::DescriptorType::eStorageImage, 1000},
        {vk::DescriptorType::eUniformTexelBuffer, 1000},
        {vk::DescriptorType::eStorageTexelBuffer, 1000},
        {vk::DescriptorType::eUniformBuffer, 1000},
        {vk::DescriptorType::eStorageBuffer, 1000},
        {vk::DescriptorType::eUniformBufferDynamic, 1000},
        {vk::DescriptorType::eStorageBufferDynamic, 1000},
        {vk::DescriptorType::eInputAttachment, 1000}};
    vk::DescriptorPoolCreateInfo poolCreate;
    poolCreate.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolCreate.maxSets = 1000;
    poolCreate.poolSizeCount = std::size(pool_sizes);
    poolCreate.pPoolSizes = pool_sizes;

    imguiDescriptorPool = device.createDescriptorPool(poolCreate);

    ImGui::CreateContext();
    presentation->initImgui();

    VkFormat format = (VkFormat)TextureFormat::RGBA16;
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = *instance;
    init_info.PhysicalDevice = *chosenGPU;
    init_info.Device = *device;
    init_info.Queue = graphicsQueue;
    init_info.DescriptorPool = *imguiDescriptorPool;
    init_info.MinImageCount = FRAMES_IN_FLIGHT;
    init_info.ImageCount = FRAMES_IN_FLIGHT;
    init_info.UseDynamicRendering = true;

    init_info.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);
}

void Engine::update() {}

CommandBuffer Engine::initFrame() {
    if (shouldRegenerate) {
        regenerate();
        shouldRegenerate = false;
    }
    auto& frame = frames[frameCounter % FRAMES_IN_FLIGHT];
    static_cast<void>(
        device.waitForFences({*frame.renderFence}, true, 10000000000000));
    device.resetFences({*frame.renderFence});

    std::pair<vk::Result, uint32_t> result;
    try {
        result = swapchain.swapchain.acquireNextImage(
            10000000000000, *frame.swapchainSemaphore);
    } catch (vk::OutOfDateKHRError& exc) {
        shouldRegenerate = true;
        frameCounter++;
        return val::CommandBuffer(*this, vk::CommandBuffer(nullptr));
    }

    imageIndex = result.second;

    auto cmd = CommandBuffer(*this, *frame.commandBuffer);
    cmd.begin();

    frame.deletionQueue.clear();
    frame.deletionQueue = std::move(deletionQueue);
    return cmd;
}

void Engine::submitFrame(Texture* backbuffer) {
    auto& frame = frames[frameCounter % FRAMES_IN_FLIGHT];

    auto cmd = CommandBuffer(*this, *frame.commandBuffer);
    auto image = swapchain.images[imageIndex];
    if (backbuffer != nullptr) {
        if (initConfig.useImGUI) {
            Texture* fb[1] = {backbuffer};
            cmd.beginPass(std::span(std::span(fb)));

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                            *frame.commandBuffer);
            cmd.endPass();
        }
        cmd.transitionImage(backbuffer->image, vk::RemainingMipLevels,
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eTransferSrcOptimal);
        cmd.transitionImage(image, vk::RemainingMipLevels,
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eTransferDstOptimal);

        vk::ImageBlit2 region;
        region.srcSubresource.layerCount = 1;
        region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.srcOffsets[0] = {.x = 0, .y = 0, .z = 0};
        region.srcOffsets[1] = {.x = (int32_t)backbuffer->size.w,
                                .y = (int32_t)backbuffer->size.h,
                                .z = (int32_t)1};

        region.dstSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.dstOffsets[0] = {.x = 0, .y = 0, .z = 0};
        region.dstOffsets[1] = {.x = (int32_t)windowSize.w,
                                .y = (int32_t)windowSize.h,
                                .z = (int32_t)1};

        vk::BlitImageInfo2 blitInfo;
        blitInfo.srcImage = backbuffer->image;
        blitInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
        blitInfo.filter = vk::Filter::eNearest;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &region;
        blitInfo.dstImage = image;
        blitInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;

        frame.commandBuffer.blitImage2(blitInfo);
    }

    cmd.transitionImage(swapchain.images[imageIndex], vk::RemainingMipLevels,
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::ePresentSrcKHR);

    frame.commandBuffer.end();

    vk::CommandBufferSubmitInfo commandBufferSubmitInfo;
    commandBufferSubmitInfo.commandBuffer = *frame.commandBuffer;

    vk::SemaphoreSubmitInfo waitInfo, signalInfo;

    waitInfo.semaphore = *frame.swapchainSemaphore;
    waitInfo.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

    signalInfo.semaphore = *frame.renderSemaphore;
    signalInfo.stageMask = vk::PipelineStageFlagBits2::eAllGraphics;

    vk::SubmitInfo2 submitInfo;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;

    graphicsQueue.submit2({submitInfo}, *frame.renderFence);

    auto sw = *swapchain.swapchain;

    vk::PresentInfoKHR presentInfo;
    presentInfo.swapchainCount = 1;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pSwapchains = &sw;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &(*frame.renderSemaphore);

    try {
        static_cast<void>(graphicsQueue.presentKHR(presentInfo));
    } catch (vk::OutOfDateKHRError& exc) {
        shouldRegenerate = true;
    }

    frameCounter++;
}

Texture* Engine::createTexture(Size size, TextureFormat format,
                               TextureSampler sampling, uint32_t mipLevels,
                               VkImageUsageFlags usage) {
    assert(mipLevels > 0 && mipLevels <= 32);

    Texture* texture = texturePool.allocate();

    texture->size = size;
    texture->format = format;
    texture->sampler = sampling;
    texture->mipLevels = mipLevels;

    VkImageCreateInfo imagecreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imagecreateInfo.format = (VkFormat)format;
    imagecreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imagecreateInfo.extent = {.width = size.w, .height = size.h, .depth = 1};
    imagecreateInfo.mipLevels = mipLevels;
    imagecreateInfo.arrayLayers = 1;
    imagecreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imagecreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imagecreateInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT |
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            (format != TextureFormat::DEPTH32
                                 ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                 : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VmaAllocationCreateInfo vmaAlloc = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags =
            VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

    texture->image.init(vma, imagecreateInfo, vmaAlloc);

    vk::ImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.image = texture->image;
    viewCreateInfo.format = vk::Format(format);
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.subresourceRange.levelCount = mipLevels;
    viewCreateInfo.subresourceRange.aspectMask =
        (format != TextureFormat::DEPTH32) ? vk::ImageAspectFlagBits::eColor
                                           : vk::ImageAspectFlagBits::eDepth;
    viewCreateInfo.viewType = vk::ImageViewType::e2D;
    texture->imageView = device.createImageView(viewCreateInfo);

    texture->bindPoint = bindings.bindTexture(*texture->imageView, sampling);

    return texture;
}

CPUBuffer* Engine::createCpuBuffer(size_t size) {
    VkBufferCreateInfo bufferInfo = {.sType =
                                         VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.pNext = nullptr;
    bufferInfo.size = size;

    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    auto buffer = cpuBufferPool.allocate();

    buffer->buffer = raii::Buffer(vma, bufferInfo, vmaAllocInfo);
    buffer->size = size;

    return buffer;
}

StorageBuffer* Engine::createStorageBuffer(uint32_t size, vk::BufferUsageFlagBits usage) {
    VkBufferCreateInfo bufferInfo = {.sType =
                                         VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.pNext = nullptr;
    bufferInfo.size = size;

    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT | (VkBufferUsageFlagBits)usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    auto buffer = bufferPool.allocate();
    buffer->buffer = raii::Buffer(vma, bufferInfo, vmaallocInfo);
    buffer->size = size;
    buffer->bindPoint = bindings.bindStorageBuffer(buffer->buffer);

    return buffer;
}

Mesh* Engine::createMesh(size_t verticesSize, uint32_t indicesCount) {
    auto mesh = meshPool.allocate();
    mesh->indicesCount = indicesCount;
    mesh->verticesSize = verticesSize;

    VkBufferCreateInfo bufferInfo = {.sType =
                                         VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.pNext = nullptr;
    bufferInfo.size = verticesSize;

    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    mesh->vertices = raii::Buffer(vma, bufferInfo, vmaallocInfo);

    bufferInfo.size = indicesCount * sizeof(uint32_t);

    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    mesh->indices = raii::Buffer(vma, bufferInfo, vmaallocInfo);

    return mesh;
}
}  // namespace val

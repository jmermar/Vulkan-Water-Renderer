#include "binding.hpp"

#include <vector>

#include "types.hpp"

namespace val {

constexpr uint32_t TEXTURE_BIND = 0;
constexpr uint32_t STORAGE_BIND = 1;

constexpr size_t MAX_DESCRIPTORS_PER_TYPE = 4096;

size_t getFirstFree(std::vector<bool>& v) {
    int i = 0;
    for (auto b : v) {
        if (!b) {
            v[i] = true;
            return i + 1;
        }
        i++;
    }
    v.push_back(true);
    return v.size();
}

void GlobalBinding::init(const vk::raii::Device& device,
                         const vk::PhysicalDeviceProperties& properties) {
    this->device = *device;
    std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
    std::vector<vk::DescriptorBindingFlags> bindingFlags;
    auto& textureBind = layoutBindings.emplace_back();
    textureBind.binding = TEXTURE_BIND;
    textureBind.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    textureBind.descriptorCount =
        properties.limits.maxDescriptorSetSampledImages;
    textureBind.stageFlags = vk::ShaderStageFlagBits::eAll;
    bindingFlags.push_back(vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind);

    auto& storageBind = layoutBindings.emplace_back();
    storageBind.binding = STORAGE_BIND;
    storageBind.descriptorType = vk::DescriptorType::eStorageBuffer;
    storageBind.descriptorCount =
        properties.limits.maxDescriptorSetStorageBuffers;
    storageBind.stageFlags = vk::ShaderStageFlagBits::eAll;
    bindingFlags.push_back(vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind);

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.flags =
        vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
    layoutCreateInfo.pBindings = layoutBindings.data();
    layoutCreateInfo.bindingCount = layoutBindings.size();

    vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreate;
    bindingFlagsCreate.bindingCount = bindingFlags.size();
    bindingFlagsCreate.pBindingFlags = bindingFlags.data();

    layoutCreateInfo.pNext = &bindingFlagsCreate;

    layout = device.createDescriptorSetLayout(layoutCreateInfo);

    // Create pool

    std::vector<vk::DescriptorPoolSize> sizes;
    auto* size = &sizes.emplace_back();
    size->type = vk::DescriptorType::eCombinedImageSampler;
    size->descriptorCount = MAX_DESCRIPTORS_PER_TYPE;

    size = &sizes.emplace_back();
    size->type = vk::DescriptorType::eStorageBuffer;
    size->descriptorCount = MAX_DESCRIPTORS_PER_TYPE;

    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind |
                           vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolCreateInfo.maxSets = MAX_DESCRIPTORS_PER_TYPE * sizes.size();
    poolCreateInfo.poolSizeCount = sizes.size();
    poolCreateInfo.pPoolSizes = sizes.data();

    pool = device.createDescriptorPool(poolCreateInfo);

    vk::DescriptorSetAllocateInfo descAllocInfo;
    descAllocInfo.descriptorPool = *pool;
    descAllocInfo.descriptorSetCount = 1;
    descAllocInfo.pSetLayouts = &(*layout);

    descriptorSet = std::move(device.allocateDescriptorSets(descAllocInfo)[0]);

    // Create samplers

    vk::SamplerCreateInfo sampler;
    sampler.magFilter = sampler.minFilter = vk::Filter::eNearest;
    sampler.maxLod = 32;
    nearestSampler = device.createSampler(sampler);

    sampler.magFilter = sampler.minFilter = vk::Filter::eLinear;
    linearSampler = device.createSampler(sampler);
}

BindPoint<Texture> GlobalBinding::bindTexture(vk::ImageView texture,
                                              TextureSampler sampling) {
    uint32_t bindPoint = getFirstFree(storageBinds);
    vk::WriteDescriptorSet write;

    vk::DescriptorImageInfo imageInfo;

    imageInfo.imageView = texture;
    imageInfo.sampler =
        sampling == TextureSampler::LINEAR ? *linearSampler : *nearestSampler;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    write.dstSet = *descriptorSet;
    write.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    write.dstBinding = TEXTURE_BIND;
    write.dstArrayElement = bindPoint;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    device.updateDescriptorSets({write}, {});

    return {bindPoint};
}

BindPoint<StorageBuffer> GlobalBinding::bindStorageBuffer(
    vk::Buffer storageBuffer) {
    uint32_t bindPoint = getFirstFree(storageBinds);
    vk::WriteDescriptorSet write;

    vk::DescriptorBufferInfo bufferInfo;

    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    bufferInfo.buffer = storageBuffer;

    write.dstSet = *descriptorSet;
    write.descriptorType = vk::DescriptorType::eStorageBuffer;
    write.dstBinding = STORAGE_BIND;
    write.dstArrayElement = bindPoint;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;

    device.updateDescriptorSets({write}, {});

    return {bindPoint};
}
}  // namespace val

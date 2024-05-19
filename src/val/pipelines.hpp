#pragma once

#include "gpu_resources.hpp"
#include "system.hpp"
#include "types.hpp"

namespace val {
class GraphicsPipeline {
    friend class CommandBuffer;
    friend class PipelineBuilder;

   private:
    vk::raii::PipelineLayout layout{nullptr};
    vk::raii::Pipeline pipeline{nullptr};

    GraphicsPipeline(vk::raii::Device& device,
                     const vk::GraphicsPipelineCreateInfo& pipelineInfo,
                     const vk::PipelineLayoutCreateInfo& layoutInfo) {
        layout = device.createPipelineLayout(layoutInfo);
        auto pci = pipelineInfo;
        pci.layout = *layout;
        pipeline = device.createGraphicsPipeline(nullptr, pci);
    }

   public:
    GraphicsPipeline() = default;
};

class ComputePipeline {
    friend class CommandBuffer;
    friend class ComputePipelineBuilder;

   private:
    vk::raii::PipelineLayout layout{nullptr};
    vk::raii::Pipeline pipeline{nullptr};

    ComputePipeline(vk::raii::Device& device,
                     const vk::ComputePipelineCreateInfo& pipelineInfo,
                     const vk::PipelineLayoutCreateInfo& layoutInfo) {
        layout = device.createPipelineLayout(layoutInfo);
        auto pci = pipelineInfo;
        pci.layout = *layout;
        pipeline = device.createComputePipeline(nullptr, pci);
    }

   public:
    ComputePipeline() = default;
};

class ComputePipelineBuilder {
    private:
    Engine& engine;
    vk::PushConstantRange pushConstant;

    vk::PipelineShaderStageCreateInfo stage;
    vk::raii::ShaderModule shaderModule{nullptr};

public:
ComputePipelineBuilder(Engine& engine) : engine(engine) {}
    ComputePipelineBuilder& setShader(const std::span<uint8_t> shaderData);
    template <typename T>
    ComputePipelineBuilder& setPushConstant() {
        pushConstant.size = sizeof(T);
        pushConstant.offset = 0;

        return *this;
    }
    ComputePipeline build();
};

class PipelineBuilder {
   private:
    Engine& engine;
    // PipelineLayout
    vk::PushConstantRange pushConstant{};

    // Pipeline
    vk ::DynamicState dynamicStates[2] = {vk::DynamicState::eViewport,
                                          vk::DynamicState::eScissor};
    std::vector<vk::Format> colorAttachmentFormats;
    std::vector<vk::PipelineColorBlendAttachmentState> blendAttachment;

    uint32_t stride = 0;
    std::vector<vk::VertexInputAttributeDescription> attributes;
    std::vector<vk::raii::ShaderModule> modules;
    std::vector<vk::PipelineShaderStageCreateInfo> stages;

    vk::PipelineVertexInputStateCreateInfo vertexInput;
    vk::PipelineInputAssemblyStateCreateInfo assembly;
    vk::PipelineTessellationStateCreateInfo tessellation;
    vk::PipelineViewportStateCreateInfo viewport;
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    vk::PipelineMultisampleStateCreateInfo multisample;
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    vk::PipelineColorBlendStateCreateInfo colorBlend;
    vk::PipelineDynamicStateCreateInfo dynamicState;
    vk::PipelineRenderingCreateInfo renderInfo;

   public:
    PipelineBuilder(Engine& engine);

    template <typename T>
    PipelineBuilder& setPushConstant() {
        pushConstant.size = sizeof(T);
        pushConstant.offset = 0;

        return *this;
    }

    PipelineBuilder& addStage(const std::span<uint8_t> shaderData,
                              ShaderStage stage);
    PipelineBuilder& clearStages() {
        stages.clear();
        modules.clear();
        return *this;
    }
    PipelineBuilder& setVertexStride(uint32_t stride) {
        this->stride = stride;
        return *this;
    }
    PipelineBuilder& addVertexInputAttribute(uint32_t offset,
                                             VertexInputFormat format);
    PipelineBuilder& clearVertexInputAttributes() {
        attributes.clear();
        return *this;
    }

    PipelineBuilder& fillTriangles();
    PipelineBuilder& drawTriangleLines();
    PipelineBuilder& drawLines();

    PipelineBuilder& setCullMode(PolygonCullMode mode) {
        rasterizer.polygonMode = vk::PolygonMode(mode);
        rasterizer.lineWidth = 1.f;
        return *this;
    }

    PipelineBuilder& enableTessellation(uint32_t controlPoints) {
        assembly.topology = vk::PrimitiveTopology::ePatchList;
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        tessellation.patchControlPoints = controlPoints;
        return *this;
    }

    PipelineBuilder& addColorAttachment(TextureFormat format);
    PipelineBuilder& clearColorAttachments() {
        colorAttachmentFormats.clear();
        blendAttachment.clear();
        return *this;
    }

    PipelineBuilder& disableMultisampling();

    PipelineBuilder& disableDepthTest();
    PipelineBuilder& depthTestRead();
    PipelineBuilder& depthTestReadWrite();

    GraphicsPipeline build();
};
}  // namespace val

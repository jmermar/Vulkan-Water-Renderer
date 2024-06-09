#include "pipelines.hpp"

namespace val {
PipelineBuilder::PipelineBuilder(Engine &engine) : engine(engine) {
  rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
  renderInfo.depthAttachmentFormat = vk::Format(TextureFormat::DEPTH32);
  pushConstant.stageFlags = vk::ShaderStageFlagBits::eAll;

  dynamicState.dynamicStateCount = 2;
  dynamicState.pDynamicStates = dynamicStates;

  drawLines();
  setCullMode(PolygonCullMode::NONE);
  disableMultisampling();
  disableDepthTest();
}

PipelineBuilder &PipelineBuilder::addStage(const std::span<uint8_t> shaderData,
                                           ShaderStage stage) {
  vk::ShaderModuleCreateInfo moduleCreate;
  moduleCreate.pCode = (uint32_t *)shaderData.data();
  moduleCreate.codeSize = shaderData.size();
  rasterizer.lineWidth = 1.f;

  modules.push_back(engine.device.createShaderModule(moduleCreate));

  vk::PipelineShaderStageCreateInfo stageInfo;
  stageInfo.pName = "main";
  stageInfo.module = *modules.back();
  stageInfo.stage = vk::ShaderStageFlagBits(stage);
  stages.push_back(stageInfo);
  return *this;
}

PipelineBuilder &
PipelineBuilder::addVertexInputAttribute(uint32_t offset,
                                         VertexInputFormat format) {
  vk::VertexInputAttributeDescription attr;
  attr.format = vk::Format(format);
  attr.location = attributes.size();
  attr.offset = offset;
  attributes.push_back(attr);
  return *this;
}

PipelineBuilder &PipelineBuilder::fillTriangles() {
  assembly.topology = vk::PrimitiveTopology::eTriangleList;
  rasterizer.polygonMode = vk::PolygonMode::eFill;
  return *this;
}

PipelineBuilder &PipelineBuilder::drawTriangleLines() {
  assembly.topology = vk::PrimitiveTopology::eTriangleList;
  rasterizer.polygonMode = vk::PolygonMode::eLine;
  return *this;
}

PipelineBuilder &PipelineBuilder::drawLines() {
  assembly.topology = vk::PrimitiveTopology::eLineList;
  rasterizer.polygonMode = vk::PolygonMode::eLine;
  return *this;
}

PipelineBuilder &PipelineBuilder::addColorAttachment(TextureFormat format) {
  vk::PipelineColorBlendAttachmentState blend;
  blend.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eA | vk::ColorComponentFlagBits::eB;
  blend.blendEnable = false;

  colorAttachmentFormats.push_back(vk::Format(format));
  blendAttachment.push_back(blend);

  return *this;
}

PipelineBuilder &PipelineBuilder::disableMultisampling() {
  multisample.sampleShadingEnable = false;
  multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;
  multisample.minSampleShading = 1.0f;
  multisample.pSampleMask = nullptr;
  multisample.alphaToCoverageEnable = false;
  multisample.alphaToOneEnable = false;
  return *this;
}

PipelineBuilder &PipelineBuilder::disableDepthTest() {
  depthStencil.depthTestEnable = false;
  depthStencil.depthWriteEnable = false;
  depthStencil.depthCompareOp = vk::CompareOp::eNever;
  depthStencil.depthBoundsTestEnable = false;
  depthStencil.stencilTestEnable = false;
  depthStencil.minDepthBounds = 0.f;
  depthStencil.maxDepthBounds = 1.f;
  return *this;
}

PipelineBuilder &PipelineBuilder::depthTestReadWrite() {
  depthStencil.depthTestEnable = true;
  depthStencil.depthWriteEnable = true;
  depthStencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
  depthStencil.depthBoundsTestEnable = false;
  depthStencil.stencilTestEnable = false;
  depthStencil.minDepthBounds = 0.f;
  depthStencil.maxDepthBounds = 1.f;
  return *this;
}

PipelineBuilder &PipelineBuilder::depthTestRead() {
  depthStencil.depthTestEnable = true;
  depthStencil.depthWriteEnable = false;
  depthStencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
  depthStencil.depthBoundsTestEnable = false;
  depthStencil.stencilTestEnable = false;
  depthStencil.minDepthBounds = 0.f;
  depthStencil.maxDepthBounds = 1.f;
  return *this;
}

PipelineBuilder &PipelineBuilder::setTessellation(uint32_t controlPoints) {
  tessellation.patchControlPoints = controlPoints;
  return *this;
}

GraphicsPipeline PipelineBuilder::build() {
  vk::PipelineViewportStateCreateInfo viewport;
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  vk::VertexInputBindingDescription vertexBind;
  vertexBind.stride = stride;
  vertexBind.binding = 0;

  vertexInput.vertexBindingDescriptionCount = 1;
  vertexInput.pVertexBindingDescriptions = &vertexBind;
  vertexInput.pVertexAttributeDescriptions = attributes.data();
  vertexInput.vertexAttributeDescriptionCount = attributes.size();

  colorBlend.logicOpEnable = false;
  colorBlend.attachmentCount = blendAttachment.size();
  colorBlend.pAttachments = blendAttachment.data();

  renderInfo.colorAttachmentCount = colorAttachmentFormats.size();
  renderInfo.pColorAttachmentFormats = colorAttachmentFormats.data();

  vk::GraphicsPipelineCreateInfo createInfo;
  createInfo.stageCount = stages.size();
  createInfo.pStages = stages.data();
  createInfo.pVertexInputState = &vertexInput;
  createInfo.pInputAssemblyState = &assembly;
  createInfo.pViewportState = &viewport;
  createInfo.pRasterizationState = &rasterizer;
  createInfo.pMultisampleState = &multisample;
  createInfo.pColorBlendState = &colorBlend;
  createInfo.pDepthStencilState = &depthStencil;
  createInfo.pDynamicState = &dynamicState;
  createInfo.pNext = &renderInfo;
  createInfo.pTessellationState = &tessellation;

  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pushConstant;
  auto descLayout = engine.bindings.getLayout();
  layoutInfo.pSetLayouts = &descLayout;
  layoutInfo.setLayoutCount = 1;
  return GraphicsPipeline(engine.device, createInfo, layoutInfo);
}
ComputePipelineBuilder &
ComputePipelineBuilder::setShader(const std::span<uint8_t> shaderData) {
  vk::ShaderModuleCreateInfo moduleCreate;
  moduleCreate.pCode = (uint32_t *)shaderData.data();
  moduleCreate.codeSize = shaderData.size();

  shaderModule = engine.device.createShaderModule(moduleCreate);

  stage.pName = "main";
  stage.module = *shaderModule;
  stage.stage = vk::ShaderStageFlagBits::eCompute;
  return *this;
}

ComputePipeline ComputePipelineBuilder::build() {
  pushConstant.stageFlags = vk::ShaderStageFlagBits::eAll;
  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.pushConstantRangeCount = 1;
  layoutInfo.pPushConstantRanges = &pushConstant;
  auto descLayout = engine.bindings.getLayout();
  layoutInfo.pSetLayouts = &descLayout;
  layoutInfo.setLayoutCount = 1;

  vk::ComputePipelineCreateInfo computeCreateInfo;
  computeCreateInfo.stage = stage;

  return ComputePipeline(engine.device, computeCreateInfo, layoutInfo);
}
} // namespace val
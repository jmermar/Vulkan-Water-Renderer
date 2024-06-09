#include "WaterRenderer.hpp"

constexpr size_t WATER_RESOLUTION = 2048;
constexpr float WATER_PLANE_SIZE = 100;

constexpr size_t PATCHES = 128;
constexpr size_t NUM_PATCHES = PATCHES * PATCHES;
constexpr size_t PATCHES_PER_GROUP = 256;
constexpr size_t NUM_GROUPS = NUM_PATCHES / PATCHES_PER_GROUP;

struct WaterPushConstants {
  glm::mat4 projView;
  glm::mat4 view;
  glm::vec3 camPos;
  val::BindPoint<val::Texture> skybox;
  float time;
};

struct DrawIndirectCommand {
  uint32_t vertexCount;
  uint32_t instanceCount;
  uint32_t firstVertex;
  uint32_t firstInstance;
};

struct ComputePushConstants {
  glm::vec3 camPos;
  val::BindPoint<val::StorageBuffer> waterPatches;
  val::BindPoint<val::StorageBuffer> drawIndirectCommand;
};

WaterRenderer::WaterRenderer(val::Engine &engine,
                             val::BufferWriter &bufferWritter)
    : engine(engine) {

  auto vertShader = file::readBinary("shaders/water.vert.spv");
  auto fragShader = file::readBinary("shaders/water.frag.spv");
  auto teseShader = file::readBinary("shaders/water.tese.spv");
  auto tescShader = file::readBinary("shaders/water.tesc.spv");

  val::PipelineBuilder builder(engine);
  pipeline = builder.setPushConstant<WaterPushConstants>()
                 .addVertexInputAttribute(0, val::VertexInputFormat::FLOAT4)
                 .addColorAttachment(val::TextureFormat::RGBA16)
                 .depthTestReadWrite()
                 .addStage(std::span(vertShader), val::ShaderStage::VERTEX)
                 .addStage(std::span(fragShader), val::ShaderStage::FRAGMENT)
                 .addStage(std::span(teseShader),
                           val::ShaderStage::TESSELATION_EVALUATION)
                 .addStage(std::span(tescShader),
                           val::ShaderStage::TESSELATION_CONTROL)
                 .setTessellation(4)
                 .tessellationFill()
                 .setVertexStride(sizeof(glm::vec4))
                 .build();

  auto compShader = file::readBinary("shaders/water.comp.spv");

  val::ComputePipelineBuilder cpBuild(engine);

  waterPatches =
      engine.createStorageBuffer(NUM_PATCHES * 4 * sizeof(glm::vec4),
                                 vk::BufferUsageFlagBits::eVertexBuffer);

  patchGenerator = cpBuild.setShader(compShader)
                       .setPushConstant<ComputePushConstants>()
                       .build();
  drawIndirectCommand = engine.createStorageBuffer(
      sizeof(DrawIndirectCommand), vk::BufferUsageFlagBits::eIndirectBuffer);
}

WaterRenderer::~WaterRenderer() {
  engine.destroyStorageBuffer(waterPatches);
  engine.destroyStorageBuffer(drawIndirectCommand);
}

void WaterRenderer::generatePatches(RenderState &rs) {
  auto &cmd = *rs.cmd;

  cmd.bindPipeline(patchGenerator);

  ComputePushConstants computePushConstants;
  computePushConstants.camPos = rs.camPos;
  computePushConstants.drawIndirectCommand = drawIndirectCommand->bindPoint;
  computePushConstants.waterPatches = waterPatches->bindPoint;

  cmd.pushConstants(patchGenerator, computePushConstants);

  auto cmdb = cmd.cmd;

  cmdb.dispatch(NUM_GROUPS, 1, 1);
}

void WaterRenderer::renderWater(RenderState &rs) {
  auto &cmd = *rs.cmd;
  auto cmdb = cmd.cmd;

  cmd.beginPass(std::span(&rs.colorBuffer, 1), rs.depthBuffer, true);
  WaterPushConstants pc;
  pc.projView = rs.projectionMatrix * rs.viewMatrix;
  pc.time = rs.time;
  pc.camPos = rs.camPos;
  pc.view = rs.viewMatrix;
  pc.skybox = rs.ambientMap->bindPoint;

  cmd.bindPipeline(pipeline);
  cmd.pushConstants(pipeline, pc);
  cmd.setViewport({0, 0, rs.colorBuffer->size.w, rs.colorBuffer->size.h});
  cmd.bindVertexBuffer(waterPatches);
  cmdb.drawIndirect(drawIndirectCommand->buffer, 0, 1,
                    sizeof(DrawIndirectCommand));

  cmd.endPass();
}

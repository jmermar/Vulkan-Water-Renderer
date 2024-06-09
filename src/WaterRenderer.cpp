#include "WaterRenderer.hpp"

constexpr size_t WATER_RESOLUTION = 4096;
constexpr float WATER_PLANE_SIZE = 40;

struct WaterPushConstants {
  glm::mat4 projView;
  glm::vec3 camPos;
  val::BindPoint<val::Texture> skybox;
  float time;
};

WaterRenderer::WaterRenderer(val::Engine &engine,
                             val::BufferWriter &bufferWritter)
    : engine(engine) {

  std::vector<glm::vec3> waterMesh(WATER_RESOLUTION * WATER_RESOLUTION);

  for (int z = 0; z < WATER_RESOLUTION; z++) {
    for (int x = 0; x < WATER_RESOLUTION; x++) {
      glm::vec3 position = glm::vec3((float)x / (float)WATER_RESOLUTION, 0.f,
                                     (float)z / (float)WATER_RESOLUTION) *
                               WATER_PLANE_SIZE +
                           -glm::vec3(1, 0, 1) * WATER_PLANE_SIZE * 0.5f;

      waterMesh[z * WATER_RESOLUTION + x] = position;
    }
  }

  std::vector<uint32_t> indices;
  indices.resize((WATER_RESOLUTION - 1) * (WATER_RESOLUTION - 1) * 6);

  size_t firstIndex = 0;
  for (size_t z = 0; z < WATER_RESOLUTION - 1; z++) {
    for (size_t x = 0; x < WATER_RESOLUTION - 1; x++) {
      indices[firstIndex] = z * WATER_RESOLUTION + x;
      indices[firstIndex + 1] = (z + 1) * WATER_RESOLUTION + x;
      indices[firstIndex + 2] = (z + 1) * WATER_RESOLUTION + (x + 1);

      indices[firstIndex + 3] = z * WATER_RESOLUTION + x;
      indices[firstIndex + 4] = (z + 1) * WATER_RESOLUTION + (x + 1);
      indices[firstIndex + 5] = z * WATER_RESOLUTION + (x + 1);

      firstIndex += 6;
    }
  }

  waterPlane =
      engine.createMesh(sizeof(glm::vec3) * waterMesh.size(), indices.size());

  bufferWritter.enqueueMeshWrite(waterPlane, std::span(waterMesh),
                                 std::span(indices));

  auto vertShader = file::readBinary("shaders/water.vert.spv");
  auto fragShader = file::readBinary("shaders/water.frag.spv");

  val::PipelineBuilder builder(engine);
  pipeline = builder.setPushConstant<WaterPushConstants>()
                 .addVertexInputAttribute(0, val::VertexInputFormat::FLOAT3)
                 .addColorAttachment(val::TextureFormat::RGBA16)
                 .depthTestReadWrite()
                 .addStage(std::span(vertShader), val::ShaderStage::VERTEX)
                 .addStage(std::span(fragShader), val::ShaderStage::FRAGMENT)
                 .fillTriangles()
                 .setVertexStride(sizeof(glm::vec3))
                 .build();
}

WaterRenderer::~WaterRenderer() { engine.destroyMesh(waterPlane); }

void WaterRenderer::renderWater(RenderState &rs) {
  auto &cmd = *rs.cmd;
  auto cmdb = cmd.cmd;

  cmd.beginPass(std::span(&rs.colorBuffer, 1), rs.depthBuffer, true);
  WaterPushConstants pc;
  pc.projView = rs.projectionMatrix * rs.viewMatrix;
  pc.time = rs.time;
  pc.camPos = rs.camPos;
  pc.skybox = rs.ambientMap->bindPoint;

  cmd.bindPipeline(pipeline);
  cmd.pushConstants(pipeline, pc);
  cmd.setViewport({0, 0, rs.colorBuffer->size.w, rs.colorBuffer->size.h});
  cmd.bindMesh(waterPlane);
  cmdb.drawIndexed(waterPlane->indicesCount, 1, 0, 0, 0);

  cmd.endPass();
}

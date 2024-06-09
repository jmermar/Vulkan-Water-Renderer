#include "SkyboxRenderer.hpp"

glm::vec3 cubeVertices[6 * 4] = {
    // right face
    {1, 1, 1},
    {1, 1, -1},
    {1, -1, -1},
    {1, -1, 1},
    // left face
    {-1, 1, 1},
    {-1, 1, -1},
    {-1, -1, -1},
    {-1, -1, 1},
    // up face
    {-1, 1, 1},
    {1, 1, 1},
    {1, 1, -1},
    {-1, 1, -1},
    // down face
    {-1, -1, 1},
    {1, -1, 1},
    {1, -1, -1},
    {-1, -1, -1},
    // back face
    {-1, 1, 1},
    {1, 1, 1},
    {1, -1, 1},
    {-1, -1, 1},
    // front face
    {-1, 1, -1},
    {1, 1, -1},
    {1, -1, -1},
    {-1, -1, -1},
};

struct PushConstants {
  glm::mat4 projView;
  glm::vec3 camPos;
  val::BindPoint<val::Texture> skybox;
};

SkyboxRenderer::SkyboxRenderer(val::Engine &engine, val::BufferWriter &writer)
    : engine(engine) {
  std::string textures[6] = {
      "textures/skybox/right.bmp", "textures/skybox/left.bmp",
      "textures/skybox/top.bmp",   "textures/skybox/bottom.bmp",
      "textures/skybox/front.bmp", "textures/skybox/back.bmp"};
  for (size_t i = 0; i < 6; i++) {
    auto image = file::loadImage(textures[i]);
    if (!skybox) {
      skybox = engine.createCubemap(image.size, val::TextureFormat::RGBA8);
    }
    writer.enqueueTextureWrite(skybox, image.data.data(), i);
  }

  cube = engine.createMesh(6 * 4 * sizeof(glm::vec3), 6 * 6);

  uint32_t indices[6 * 6];
  for (size_t i = 0; i < 6; i++) {
    auto firstIndex = i * 6;
    auto firstVertex = i * 4;
    indices[firstIndex] = firstVertex;
    indices[firstIndex + 1] = firstVertex + 1;
    indices[firstIndex + 2] = firstVertex + 2;

    indices[firstIndex + 3] = firstVertex;
    indices[firstIndex + 4] = firstVertex + 2;
    indices[firstIndex + 5] = firstVertex + 3;
  }

  writer.enqueueMeshWrite(cube, std::span(cubeVertices, 6 * 4),
                          std::span(indices, 6 * 6));

  auto vertShader = file::readBinary("shaders/skybox.vert.spv");
  auto fragShader = file::readBinary("shaders/skybox.frag.spv");

  val::PipelineBuilder builder(engine);
  pipeline = builder.setPushConstant<PushConstants>()
                 .addVertexInputAttribute(0, val::VertexInputFormat::FLOAT3)
                 .addColorAttachment(val::TextureFormat::RGBA16)
                 .disableDepthTest()
                 .addStage(std::span(vertShader), val::ShaderStage::VERTEX)
                 .addStage(std::span(fragShader), val::ShaderStage::FRAGMENT)
                 .fillTriangles()
                 .setVertexStride(sizeof(glm::vec3))
                 .build();
}

SkyboxRenderer::~SkyboxRenderer() {
  engine.destroyMesh(cube);
  engine.freeTexture(skybox);
}

void SkyboxRenderer::renderSkybox(RenderState &rs) {
  auto &cmd = *rs.cmd;
  auto cmdb = cmd.cmd;

  cmd.beginPass(std::span(&rs.colorBuffer, 1));
  PushConstants pc;
  pc.projView = rs.projectionMatrix * rs.viewMatrix;
  pc.camPos = rs.camPos;
  pc.skybox = skybox->bindPoint;

  cmd.bindPipeline(pipeline);
  cmd.pushConstants(pipeline, pc);
  cmd.setViewport({0, 0, rs.colorBuffer->size.w, rs.colorBuffer->size.h});
  cmd.bindMesh(cube);
  cmdb.drawIndexed(cube->indicesCount, 1, 0, 0, 0);

  cmd.endPass();
}
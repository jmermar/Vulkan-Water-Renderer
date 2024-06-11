#include "PostProcess.hpp"

struct PushConstants {
  glm::mat4 invProj;
  glm::mat4 invView;
  val::BindPoint<val::Texture> depth;
  val::BindPoint<val::Texture> source;
  float pad[2];
};

PostProcess::PostProcess(val::Engine &engine) : engine(engine) {
  auto vertShader = file::readBinary("shaders/postprocess.vert.spv");
  auto fragShader = file::readBinary("shaders/postprocess.frag.spv");

  val::PipelineBuilder builder(engine);
  pipeline = builder.setPushConstant<PushConstants>()
                 .addColorAttachment(val::TextureFormat::RGBA16)
                 .disableDepthTest()
                 .addStage(std::span(vertShader), val::ShaderStage::VERTEX)
                 .addStage(std::span(fragShader), val::ShaderStage::FRAGMENT)
                 .fillTriangles()
                 .build();
}

void PostProcess::renderPostProcess(RenderState &rs, val::Texture *finalImage) {
  auto &cmd = *rs.cmd;
  auto cmdb = cmd.cmd;

  cmd.beginPass(std::span(&finalImage, 1));
  PushConstants pc;
  pc.invProj = glm::inverse(rs.projectionMatrix);
  pc.invView = glm::inverse(rs.viewMatrix);
  pc.depth = rs.depthBuffer->bindPoint;
  pc.source = rs.colorBuffer->bindPoint;

  cmd.bindPipeline(pipeline);
  cmd.pushConstants(pipeline, pc);
  cmd.setViewport({0, 0, rs.colorBuffer->size.w, rs.colorBuffer->size.h});
  cmdb.draw(6, 1, 0, 0);

  cmd.endPass();
}

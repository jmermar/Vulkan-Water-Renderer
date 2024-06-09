#pragma once

#include "types.hpp"

class SkyboxRenderer {
private:
  val::Engine &engine;
  val::GraphicsPipeline pipeline{};
  val::Texture *skybox{};
  val::Mesh *cube;

public:
  SkyboxRenderer(val::Engine &engine, val::BufferWriter &writer);
  ~SkyboxRenderer();

  void renderSkybox(RenderState &rs);
};
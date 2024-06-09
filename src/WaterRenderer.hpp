#pragma once

#include "types.hpp"

class WaterRenderer {
private:
  val::Engine &engine;
  val::Mesh *waterPlane;
  val::GraphicsPipeline pipeline;

public:
  WaterRenderer(val::Engine &engine, val::BufferWriter &bufferWritter);
  ~WaterRenderer();

  void renderWater(RenderState &rs);
};
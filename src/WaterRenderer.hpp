#pragma once

#include "types.hpp"

class WaterRenderer {
private:
  val::Engine &engine;
  val::StorageBuffer *waterPatches;
  val::StorageBuffer *drawIndirectCommand;
  val::GraphicsPipeline pipeline;
  val::ComputePipeline patchGenerator;

public:
  WaterRenderer(val::Engine &engine, val::BufferWriter &bufferWritter);
  ~WaterRenderer();

  void generatePatches(RenderState &rs);

  void renderWater(RenderState &rs);
};
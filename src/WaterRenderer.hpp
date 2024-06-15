#pragma once

#include "types.hpp"

struct WaterMaterial {
  glm::vec4 diffuseColor = {0.f, 0.1f, 0.4f, 1.f};
  glm::vec2 baseD = {1, 0};
  float baseA = 0.1;
  float baseW = 1;

  uint32_t numFreqs = 60;
  float aMult = 0.8;
  float wMult = 1.2;

  float speed = 2;

  float baseReflectivity = 0.02;

  float roughness = 0.12;
};

class WaterRenderer {
private:
  val::Engine &engine;
  val::BufferWriter &writer;
  val::StorageBuffer *waterPatches;
  val::StorageBuffer *drawIndirectCommand;
  val::StorageBuffer *waterMaterial;
  val::GraphicsPipeline pipeline;
  val::ComputePipeline patchGenerator;

public:
  WaterRenderer(val::Engine &engine, val::BufferWriter &bufferWritter);
  ~WaterRenderer();

  void updateMaterial(const WaterMaterial &material);

  void generatePatches(RenderState &rs);

  void renderWater(RenderState &rs);
};
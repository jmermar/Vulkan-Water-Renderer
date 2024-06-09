#pragma once

#include "val/vulkan_abstraction.hpp"

struct RenderState {
  val::Texture *colorBuffer;
  val::Texture *depthBuffer;

  glm::mat4 projectionMatrix;
  glm::mat4 viewMatrix;

  val::CommandBuffer *cmd;
};
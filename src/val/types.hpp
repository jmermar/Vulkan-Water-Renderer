#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "../foundation/foundation.hpp"

namespace val {
constexpr uint32_t FRAMES_IN_FLIGHT = 2;

enum class PresentationFormat : uint32_t {
  Immediate = VK_PRESENT_MODE_IMMEDIATE_KHR,
  Mailbox = VK_PRESENT_MODE_MAILBOX_KHR,
  Fifo = VK_PRESENT_MODE_FIFO_KHR
};

enum class TextureFormat : uint32_t {
  RGBA8 = VK_FORMAT_R8G8B8A8_SRGB,
  RGBA16 = VK_FORMAT_R16G16B16A16_SFLOAT,
  DEPTH32 = VK_FORMAT_D32_SFLOAT
};

enum class TextureSampler : uint32_t {
  NEAREST = VK_FILTER_NEAREST,
  LINEAR = VK_FILTER_LINEAR
};

enum class ShaderStage : uint32_t {
  VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
  FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
  TESSELATION_CONTROL = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
  TESSELATION_EVALUATION = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
};

enum class VertexInputFormat : uint32_t {
  FLOAT = VK_FORMAT_D32_SFLOAT,
  FLOAT2 = VK_FORMAT_R32G32_SFLOAT,
  FLOAT3 = VK_FORMAT_R32G32B32_SFLOAT,
  FLOAT4 = VK_FORMAT_R32G32B32A32_SFLOAT
};

enum class PolygonCullMode : uint32_t {
  NONE = VK_CULL_MODE_NONE,
  CCW = VK_CULL_MODE_FRONT_BIT,
  CW = VK_CULL_MODE_BACK_BIT
};

struct EngineInitConfig {
  PresentationFormat presentation;
  bool useImGUI{};
  vk::PhysicalDeviceVulkan13Features features;
  vk::PhysicalDeviceVulkan12Features features12;
  vk::PhysicalDeviceFeatures features10;
};

template <typename T> struct BindPoint {
  uint32_t bind;
};

} // namespace val

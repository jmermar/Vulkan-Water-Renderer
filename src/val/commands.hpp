#pragma once

#include "gpu_resources.hpp"
namespace val {
class GraphicsPipeline;
class ComputePipeline;
class CommandBuffer {
  friend class Engine;

private:
  Engine &engine;
  CommandBuffer(Engine &e, vk::CommandBuffer cmd) : engine(e), cmd(cmd) {}

  void begin();

  void transitionImage(vk::Image image, uint32_t layer, uint32_t mipLevels,
                       vk::ImageLayout srcLayout,
                       vk::PipelineStageFlagBits2 srcStage,
                       vk::ImageLayout dstLayout,
                       vk::PipelineStageFlagBits2 dstStage, bool depth = 0);

  inline void transitionImage(vk::Image image, uint32_t layer,
                              uint32_t mipLevels, vk::ImageLayout srcLayout,
                              vk::ImageLayout dstLayout, bool depth = 0) {
    transitionImage(image, layer, mipLevels, srcLayout,
                    vk::PipelineStageFlagBits2::eAllCommands, dstLayout,
                    vk::PipelineStageFlagBits2::eAllCommands, depth);
  }

  void copyBufferToBuffer(StorageBuffer *dst, vk::Buffer src, uint32_t srcStart,
                          uint32_t dstStart, size_t size);

  void copyToTexture(Texture *t, vk::Buffer buffer,
                     vk::PipelineStageFlagBits2 srcStage =
                         vk::PipelineStageFlagBits2::eAllCommands,
                     vk::PipelineStageFlagBits2 dstStage =
                         vk::PipelineStageFlagBits2::eAllCommands,
                     uint32_t dstLayer = 0);
  void _bindPipeline(GraphicsPipeline &p);
  void _bindPipeline(ComputePipeline &p);
  void _pushConstants(GraphicsPipeline &p, const void *data, uint32_t size);
  void _pushConstants(ComputePipeline &p, const void *data, uint32_t size);

public:
  vk::CommandBuffer cmd;
  inline void transitionTexture(Texture *texture, vk::ImageLayout srcLayout,
                                vk::PipelineStageFlagBits2 srcStage,
                                vk::ImageLayout dstLayout,
                                vk::PipelineStageFlagBits2 dstStage,
                                uint32_t layer = 0) {
    transitionImage(texture->image, layer, texture->mipLevels, srcLayout,
                    srcStage, dstLayout, dstStage,
                    texture->format == TextureFormat::DEPTH32);
  }
  inline void transitionTexture(Texture *texture, vk::ImageLayout srcLayout,
                                vk::ImageLayout dstLayout, uint32_t layer = 0) {
    transitionImage(texture->image, layer, texture->mipLevels, srcLayout,
                    vk::PipelineStageFlagBits2::eAllCommands, dstLayout,
                    vk::PipelineStageFlagBits2::eAllCommands,
                    texture->format == TextureFormat::DEPTH32);
  }

  void copyToTexture(Texture *t, CPUBuffer *buffer,
                     vk::PipelineStageFlagBits2 srcStage =
                         vk::PipelineStageFlagBits2::eAllCommands,
                     vk::PipelineStageFlagBits2 dstStage =
                         vk::PipelineStageFlagBits2::eAllCommands,
                     uint32_t dstLayer = 0) {
    copyToTexture(t, buffer->buffer, srcStage, dstStage, dstLayer);
  }

  void copyToTexture(Texture *t, StorageBuffer *buffer,
                     vk::PipelineStageFlagBits2 srcStage =
                         vk::PipelineStageFlagBits2::eAllCommands,
                     vk::PipelineStageFlagBits2 dstStage =
                         vk::PipelineStageFlagBits2::eAllCommands,
                     uint32_t dstLayer = 0) {
    copyToTexture(t, buffer->buffer, srcStage, dstStage, dstLayer);
  }

  void memoryBarrier(vk::PipelineStageFlags2 srcStage,
                     vk::AccessFlags2 srcAccess,
                     vk::PipelineStageFlags2 dstStage,
                     vk::AccessFlags2 dstAccess);

  void copyTextureToTexture(Texture *src, Texture *dst, uint32_t srcLayer = 0,
                            uint32_t dstLayer = 0);

  void copyBufferToBuffer(StorageBuffer *dst, StorageBuffer *src,
                          uint32_t srcStart, uint32_t dstStart, size_t size) {
    copyBufferToBuffer(dst, src->buffer, srcStart, dstStart, size);
  }

  void copyBufferToBuffer(StorageBuffer *dst, CPUBuffer *src, uint32_t srcStart,
                          uint32_t dstStart, size_t size) {
    copyBufferToBuffer(dst, src->buffer, srcStart, dstStart, size);
  }

  void copyToMesh(Mesh *mesh, CPUBuffer *vertices, CPUBuffer *indices);

  void clearImage(vk::Image image, float r, float g, float b, float a);

  bool isValid() { return cmd != 0; }

  void generateMipMapLevels(Texture *tex);

  void beginPass(std::span<Texture *> framebuffers, Texture *depthBuffer = 0,
                 bool clearDepth = false);

  void endPass();

  void bindPipeline(GraphicsPipeline &pipeline) { _bindPipeline(pipeline); }
  void bindPipeline(ComputePipeline &pipeline) { _bindPipeline(pipeline); }
  template <typename T>
  void pushConstants(GraphicsPipeline &pipeline, const T &t) {
    _pushConstants(pipeline, &t, sizeof(T));
  }
  template <typename T>
  void pushConstants(ComputePipeline &pipeline, const T &t) {
    _pushConstants(pipeline, &t, sizeof(T));
  }
  void setViewport(const Rect &viewport) {
    vk::Viewport vp;
    vp.x = viewport.x;
    vp.y = viewport.y;
    vp.width = (float)viewport.w;
    vp.height = (float)viewport.h;
    vp.maxDepth = 1.f;
    vp.minDepth = 0.f;

    vk::Rect2D scissor;
    scissor.offset.x = viewport.x;
    scissor.offset.y = viewport.y;
    scissor.extent.width = viewport.w;
    scissor.extent.height = viewport.h;

    cmd.setViewport(0, 1, &vp);
    cmd.setScissor(0, 1, &scissor);
  };

  void bindMesh(Mesh *mesh) {
    cmd.bindIndexBuffer(mesh->indices, 0, vk::IndexType::eUint32);
    vk::Buffer vertices = mesh->vertices;
    vk::DeviceSize offset = 0;

    cmd.bindVertexBuffers(0, 1, &vertices, &offset);
  }

  void bindVertexBuffer(StorageBuffer *buffer) {
    vk::Buffer vertices = buffer->buffer;
    vk::DeviceSize offset = 0;

    cmd.bindVertexBuffers(0, 1, &vertices, &offset);
  }
};
} // namespace val

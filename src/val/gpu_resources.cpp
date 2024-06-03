#include "gpu_resources.hpp"

#include <cassert>

#include "helpers.hpp"
#include "system.hpp"
namespace val {
void BufferWriter::updateWrites(CommandBuffer &cmd) {
  cmd.memoryBarrier(
      vk::PipelineStageFlagBits2::eAllCommands,
      vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
      vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eMemoryWrite);

  for (auto &[texture, buffer, layer] : textureWrites) {
    cmd.transitionTexture(texture, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal, layer);
    cmd.copyToTexture(texture, buffer, vk::PipelineStageFlagBits2::eAllCommands,
                      vk::PipelineStageFlagBits2::eAllCommands, layer);
    cmd.transitionTexture(texture, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eShaderReadOnlyOptimal, layer);
    engine.destroyCpuBuffer(buffer);
  }
  textureWrites.clear();

  for (auto &[buffer, start, size, upload] : bufferWrites) {
    cmd.copyBufferToBuffer(buffer, upload, start, 0, size);
    engine.destroyCpuBuffer(upload);
  }

  bufferWrites.clear();

  for (auto &[mesh, vertices, indices] : meshWrites) {
    cmd.copyToMesh(mesh, vertices, indices);
    engine.destroyCpuBuffer(vertices);
    engine.destroyCpuBuffer(indices);
  }

  meshWrites.clear();

  cmd.memoryBarrier(
      vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eMemoryWrite,
      vk::PipelineStageFlagBits2::eAllCommands,
      vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead);
}

void BufferWriter::enqueueTextureWrite(Texture *tex, void *data,
                                       uint32_t layer) {
  const auto size =
      helpers::getTextureSizeFromSizeAndFormat(tex->size, tex->format);

  auto upload = engine.createCpuBuffer(size);
  engine.updateCPUBuffer(upload, data, size);

  textureWrites.push_back({.texture = tex, .buffer = upload, .layer = layer});
}

void BufferWriter::enqueueBufferWrite(StorageBuffer *buffer, void *data,
                                      uint32_t start, size_t size) {
  assert(data);
  assert(start + size <= buffer->size);

  auto upload = engine.createCpuBuffer(size);
  engine.updateCPUBuffer(upload, data, size);

  bufferWrites.push_back(
      {.buffer = buffer, .start = start, .size = size, .uploadBuffer = upload});
}

void BufferWriter::enqueueMeshWrite(Mesh *mesh, void *data, uint32_t dataSize,
                                    std::span<uint32_t> indices) {
  auto indicesSize = indices.size() * sizeof(uint32_t);
  assert(data);
  assert(dataSize == mesh->verticesSize);
  assert(indices.size() == mesh->indicesCount);
  auto verticesUpload = engine.createCpuBuffer(dataSize);
  engine.updateCPUBuffer(verticesUpload, data, dataSize);

  auto indicesUpload = engine.createCpuBuffer(indicesSize);
  engine.updateCPUBuffer(indicesUpload, indices.data(), indicesSize);

  meshWrites.push_back({.mesh = mesh,
                        .verticesUpload = verticesUpload,
                        .indicesUpload = indicesUpload});
}
} // namespace val

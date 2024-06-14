#pragma once

#include <span>

#include "raii.hpp"
#include "types.hpp"
namespace val {
struct StorageBuffer {
  BindPoint<StorageBuffer> bindPoint{};
  raii::Buffer buffer{};

  size_t size{};
};

struct Texture {
  BindPoint<Texture> bindPoint{};
  raii::Image image{};
  vk::raii::ImageView imageView{nullptr};
  Size size{};
  TextureFormat format{};
  TextureSampler sampler{};
  uint32_t mipLevels{};
  uint32_t layers{};
};

struct Mesh {
  size_t indicesCount{};
  size_t verticesSize{};
  raii::Buffer vertices{};
  raii::Buffer indices{};
};

struct CPUBuffer {
  size_t size{};
  raii::Buffer buffer;
};

class Engine;
class CommandBuffer;
class BufferWriter {
private:
  struct TextureWriteOperation {
    Texture *texture;
    CPUBuffer *buffer;
    uint32_t layer{};
  };

  struct BufferWrite {
    StorageBuffer *buffer{};
    uint32_t start{};
    size_t size{};
    CPUBuffer *uploadBuffer;
  };

  struct MeshWrite {
    Mesh *mesh;
    CPUBuffer *verticesUpload;
    CPUBuffer *indicesUpload;
  };

  std::vector<TextureWriteOperation> textureWrites;
  std::vector<BufferWrite> bufferWrites;
  std::vector<MeshWrite> meshWrites;

  Engine &engine;

public:
  BufferWriter(Engine &engine) : engine(engine) {}

  void updateWrites(CommandBuffer &cmd);

  void enqueueTextureWrite(Texture *tex, const void *data, uint32_t layer = 0);
  void enqueueBufferWrite(StorageBuffer *buffer, const void *data,
                          uint32_t start, size_t size);

  void enqueueMeshWrite(Mesh *mesh, const void *data, uint32_t dataSize,
                        std::span<uint32_t> indices);

  template <typename T>
  void enqueueMeshWrite(Mesh *mesh, std::span<T> vertices,
                        std::span<uint32_t> indices) {
    enqueueMeshWrite(mesh, vertices.data(), vertices.size() * sizeof(T),
                     indices);
  }
};
} // namespace val

#include "commands.hpp"

#include "pipelines.hpp"
namespace val {
void CommandBuffer::begin() {
  cmd.reset();
  vk::CommandBufferBeginInfo cmdBeginInfo;
  cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
  cmd.begin(cmdBeginInfo);
}

void CommandBuffer::transitionImage(vk::Image image, uint32_t layer,
                                    uint32_t mipLevels,
                                    vk::ImageLayout srcLayout,
                                    vk::PipelineStageFlagBits2 srcStage,
                                    vk::ImageLayout dstLayout,
                                    vk::PipelineStageFlagBits2 dstStage) {
  vk::ImageMemoryBarrier2KHR imageBarrier;
  imageBarrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
  imageBarrier.srcStageMask = srcStage;

  imageBarrier.dstAccessMask =
      vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
  imageBarrier.dstStageMask = dstStage;

  imageBarrier.oldLayout = srcLayout;
  imageBarrier.newLayout = dstLayout;

  vk::ImageSubresourceRange range;
  range.levelCount = mipLevels;
  range.layerCount = 1;
  range.baseArrayLayer = layer;
  range.aspectMask = vk::ImageAspectFlagBits::eColor;

  imageBarrier.subresourceRange = range;

  imageBarrier.image = image;

  vk::DependencyInfo dependencyInfo;
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers = &imageBarrier;
  cmd.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::copyToTexture(Texture *t, vk::Buffer origin,
                                  vk::PipelineStageFlagBits2 srcStage,
                                  vk::PipelineStageFlagBits2 dstStage,
                                  uint32_t dstLayer) {
  vk::BufferImageCopy copyRegion;
  copyRegion.bufferOffset = 0;
  copyRegion.bufferRowLength = 0;
  copyRegion.bufferImageHeight = 0;

  copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  copyRegion.imageSubresource.mipLevel = 0;
  copyRegion.imageSubresource.baseArrayLayer = dstLayer;
  copyRegion.imageSubresource.layerCount = 1;
  copyRegion.imageExtent.width = t->size.w;
  copyRegion.imageExtent.height = t->size.h;
  copyRegion.imageExtent.depth = 1;

  cmd.copyBufferToImage(origin, t->image, vk::ImageLayout::eTransferDstOptimal,
                        {copyRegion});

  generateMipMapLevels(t);
}

void CommandBuffer::memoryBarrier(vk::PipelineStageFlags2 srcStage,
                                  vk::AccessFlags2 srcAccess,
                                  vk::PipelineStageFlags2 dstStage,
                                  vk::AccessFlags2 dstAccess) {
  vk::MemoryBarrier2 memoryBarrier;
  memoryBarrier.srcAccessMask = srcAccess;
  memoryBarrier.dstAccessMask = dstAccess;
  memoryBarrier.srcStageMask = srcStage;
  memoryBarrier.dstStageMask = dstStage;
  vk::DependencyInfo dependencyInfo;
  dependencyInfo.memoryBarrierCount = 1;
  dependencyInfo.pMemoryBarriers = &memoryBarrier;
  cmd.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::copyTextureToTexture(Texture *src, Texture *dst,
                                         uint32_t srcLayer, uint32_t dstLayer) {
  vk::ImageBlit2 region;
  region.srcSubresource.baseArrayLayer = srcLayer;
  region.srcSubresource.layerCount = 1;
  region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  region.srcOffsets[0] = {.x = 0, .y = 0, .z = 0};
  region.srcOffsets[1] = {
      .x = (int32_t)src->size.w, .y = (int32_t)src->size.h, .z = (int32_t)1};

  region.dstSubresource.baseArrayLayer = srcLayer;
  region.dstSubresource.layerCount = 1;
  region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
  region.dstOffsets[0] = {.x = 0, .y = 0, .z = 0};
  region.dstOffsets[1] = {
      .x = (int32_t)dst->size.w, .y = (int32_t)dst->size.h, .z = (int32_t)1};

  vk::BlitImageInfo2 blitInfo;
  blitInfo.srcImage = src->image;
  blitInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
  blitInfo.filter = vk::Filter::eNearest;
  blitInfo.regionCount = 1;
  blitInfo.pRegions = &region;
  blitInfo.dstImage = dst->image;
  blitInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;

  cmd.blitImage2(blitInfo);

  generateMipMapLevels(dst);
}

void CommandBuffer::copyBufferToBuffer(StorageBuffer *dst, vk::Buffer src,
                                       uint32_t srcStart, uint32_t dstStart,
                                       size_t size) {
  vk::CopyBufferInfo2 copyInfo;
  copyInfo.srcBuffer = src;
  copyInfo.dstBuffer = dst->buffer;

  vk::BufferCopy2 region;
  region.srcOffset = srcStart;
  region.dstOffset = dstStart;
  region.size = size;
  copyInfo.regionCount = 1;
  copyInfo.pRegions = &region;
  cmd.copyBuffer2(copyInfo);
}

void CommandBuffer::copyToMesh(Mesh *mesh, CPUBuffer *vertices,
                               CPUBuffer *indices) {
  vk::CopyBufferInfo2 copyInfo;
  copyInfo.srcBuffer = vertices->buffer;
  copyInfo.dstBuffer = mesh->vertices;

  vk::BufferCopy2 region;
  region.size = vertices->size;
  copyInfo.regionCount = 1;
  copyInfo.pRegions = &region;
  cmd.copyBuffer2(copyInfo);

  copyInfo.srcBuffer = indices->buffer;
  copyInfo.dstBuffer = mesh->indices;

  region.size = indices->size;
  cmd.copyBuffer2(copyInfo);
}

void CommandBuffer::clearImage(vk::Image image, float r, float g, float b,
                               float a) {
  vk::ImageSubresourceRange range;
  range.levelCount = vk::RemainingMipLevels;
  range.layerCount = vk::RemainingArrayLayers;
  range.aspectMask = vk::ImageAspectFlagBits::eColor;

  vk::ClearColorValue color;
  color.setFloat32({r, g, b, a});
  cmd.clearColorImage(image, vk::ImageLayout::eTransferDstOptimal, color,
                      range);
}

void CommandBuffer::generateMipMapLevels(Texture *tex) {
  if (tex->mipLevels <= 1) {
    return;
  }
  vk::ImageMemoryBarrier barrier{};
  barrier.image = tex->image;
  barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
  barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mipWidth = tex->size.w;
  int32_t mipHeight = tex->size.h;

  for (uint32_t i = 1; i < tex->mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eMemoryWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                        vk::PipelineStageFlagBits::eTransfer,
                        vk::DependencyFlags(0), {}, {}, {barrier});

    vk::ImageBlit blit{};

    blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
    blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
    blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1,
                                      mipHeight > 1 ? mipHeight / 2 : 1, 1);
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    cmd.blitImage(tex->image, vk::ImageLayout::eTransferSrcOptimal, tex->image,
                  vk::ImageLayout::eTransferDstOptimal, {blit},
                  (vk::Filter)tex->sampler);

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }
}

void CommandBuffer::beginPass(std::span<Texture *> framebuffers,
                              Texture *depthBuffer, bool clearDepth) {
  Size area;
  vk::RenderingInfo renderInfo;
  std::vector<vk::RenderingAttachmentInfo> colorAttachments(
      framebuffers.size());
  vk::RenderingAttachmentInfo attachInfo, depthInfo;
  attachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
  attachInfo.loadOp = vk::AttachmentLoadOp::eLoad;
  attachInfo.storeOp = vk::AttachmentStoreOp::eStore;
  for (size_t i = 0; i < framebuffers.size(); i++) {
    auto texture = framebuffers[i];
    attachInfo.imageView = *texture->imageView;
    colorAttachments[i] = attachInfo;
    area = texture->size;
  }
  renderInfo.colorAttachmentCount = colorAttachments.size();
  renderInfo.pColorAttachments = colorAttachments.data();
  renderInfo.layerCount = 1;

  if (depthBuffer) {
    depthInfo.imageView = *depthBuffer->imageView;
    depthInfo.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depthInfo.loadOp =
        clearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    depthInfo.storeOp = vk::AttachmentStoreOp::eStore;
    depthInfo.clearValue.depthStencil.depth = 1.f;
    renderInfo.pDepthAttachment = &depthInfo;
  }

  renderInfo.renderArea = {.offset = {0, 0}, .extent = {area.w, area.h}};

  cmd.beginRendering(renderInfo);
}

void CommandBuffer::endPass() { cmd.endRendering(); }

void CommandBuffer::_bindPipeline(GraphicsPipeline &pipeline) {
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
  auto ds = *engine.bindings.descriptorSet;
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0,
                         1, &ds, 0, nullptr);
}

void CommandBuffer::_bindPipeline(ComputePipeline &pipeline) {
  cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline.pipeline);
  auto ds = *engine.bindings.descriptorSet;
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipeline.layout, 0,
                         1, &ds, 0, nullptr);
}

void CommandBuffer::_pushConstants(GraphicsPipeline &pipeline, const void *data,
                                   uint32_t size) {
  cmd.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eAll, 0, size,
                    data);
}

void CommandBuffer::_pushConstants(ComputePipeline &pipeline, const void *data,
                                   uint32_t size) {
  cmd.pushConstants(*pipeline.layout, vk::ShaderStageFlagBits::eAll, 0, size,
                    data);
}
} // namespace val

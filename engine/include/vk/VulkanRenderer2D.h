#pragma once

#include <vulkan/vulkan.h>

#include "vk/VulkanBuffer.h"
#include "vk/VulkanDescriptorSetLayout.h"
#include "vk/VulkanGraphicsPipeline.h"
#include "vk/VulkanShaderModule.h"
#include "vk/VulkanTexture2D.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace eng::vk
{

struct Color4 final
{
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct QuadDrawCommand final
{
  const VulkanTexture2D* texture = nullptr;

  float x = 0.0f;
  float y = 0.0f;
  float width = 1.0f;
  float height = 1.0f;

  float u0 = 0.0f;
  float v0 = 0.0f;
  float u1 = 1.0f;
  float v1 = 1.0f;

  Color4 color{};
};

struct VulkanRenderer2DDesc final
{
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;

  VkFormat colorFormat = VK_FORMAT_UNDEFINED;

  std::filesystem::path vertexShaderPath;
  std::filesystem::path fragmentShaderPath;

  std::uint32_t framesInFlight = 2;
  std::uint32_t maxQuadsPerBatch = 2048;
};

class VulkanRenderer2D final
{
 public:
  explicit VulkanRenderer2D(const VulkanRenderer2DDesc& desc);
  ~VulkanRenderer2D();

  VulkanRenderer2D(const VulkanRenderer2D&) = delete;
  VulkanRenderer2D& operator=(const VulkanRenderer2D&) = delete;

  VulkanRenderer2D(VulkanRenderer2D&&) = delete;
  VulkanRenderer2D& operator=(VulkanRenderer2D&&) = delete;

  [[nodiscard]] std::unique_ptr<VulkanTexture2D> CreateTexture2DFromPixels(
      std::uint32_t width, std::uint32_t height, const void* pixels, VkDeviceSize pixelsSize);

  void OnSwapchainRecreated(VkFormat colorFormat);

  void DrawQuad(float x, float y, float width, float height, Color4 color);

  void DrawTexture(
      const VulkanTexture2D& texture, float x, float y, float width, float height, Color4 tint);

  void DrawTextureRegion(
      const VulkanTexture2D& texture,
      float x,
      float y,
      float width,
      float height,
      float u0,
      float v0,
      float u1,
      float v1,
      Color4 tint);

  void ClearQueuedCommands();

  void RenderQueuedCommands(
      VkCommandBuffer commandBuffer, VkExtent2D extent, std::uint32_t frameIndex);

 private:
  VulkanShaderModuleDesc MakeShaderModuleDesc(const std::filesystem::path& path) const;
  VulkanGraphicsPipelineDesc MakePipelineDesc(VkFormat colorFormat) const;

  VulkanBufferDesc MakeFrameVertexBufferDesc() const;
  VulkanBufferDesc MakeIndexBufferDesc() const;

  VulkanDescriptorSetLayoutDesc MakeTextureDescriptorSetLayoutDesc() const;
  VulkanTexture2DDesc MakeWhiteTextureDesc() const;

  std::vector<VulkanBuffer> CreateFrameVertexBuffers() const;

  void UploadIndexBuffer();
  void UploadQueuedVertices(std::uint32_t frameIndex);

  void RecreatePipeline(VkFormat colorFormat);

  void CreateDescriptorPool();
  void DestroyDescriptorPool();

  [[nodiscard]] VkDescriptorSet AllocateTextureDescriptorSet(const VulkanTexture2D& texture);

  void RenderBatch(
      VkCommandBuffer commandBuffer,
      std::uint32_t frameIndex,
      std::size_t firstCommand,
      std::size_t commandCount);

 private:
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;

  VkCommandPool m_commandPool = VK_NULL_HANDLE;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;

  VkFormat m_colorFormat = VK_FORMAT_UNDEFINED;

  std::uint32_t m_framesInFlight = 2;
  std::uint32_t m_maxQuadsPerBatch = 2048;

  VulkanShaderModule m_vertexShader;
  VulkanShaderModule m_fragmentShader;

  VulkanDescriptorSetLayout m_textureDescriptorSetLayout;
  VulkanGraphicsPipeline m_pipeline;

  std::vector<VulkanBuffer> m_frameVertexBuffers;
  VulkanBuffer m_indexBuffer;

  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

  VulkanTexture2D m_whiteTexture;

  std::vector<QuadDrawCommand> m_quadCommands;
};

}  // namespace eng::vk
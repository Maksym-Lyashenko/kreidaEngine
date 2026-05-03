#pragma once

#include "VulkanBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanShaderModule.h"

#include <cstdint>
#include <filesystem>
#include <vector>
#include <vulkan/vulkan.h>

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
  float x = 0.0f;
  float y = 0.0f;
  float width = 1.0f;
  float height = 1.0f;
  Color4 color{};
};

struct VulkanRenderer2DDesc final
{
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

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

  void OnSwapchainRecreated(VkFormat colorFormat);

  void DrawQuad(float x, float y, float width, float height, Color4 color);
  void ClearQueuedCommands();

  void RenderQueuedCommands(
      VkCommandBuffer commandBuffer, VkExtent2D extent, std::uint32_t frameIndex);

 private:
  VulkanShaderModuleDesc MakeShaderModuleDesc(const std::filesystem::path& path) const;
  VulkanGraphicsPipelineDesc MakePipelineDesc(VkFormat colorFormat) const;

  VulkanBufferDesc MakeFrameVertexBufferDesc() const;
  VulkanBufferDesc MakeIndexBufferDesc() const;

  std::vector<VulkanBuffer> CreateFrameVertexBuffers() const;

  void UploadIndexBuffer();
  void RecreatePipeline(VkFormat colorFormat);

  void RenderBatch(
      VkCommandBuffer commandBuffer,
      VkExtent2D extent,
      std::uint32_t frameIndex,
      std::size_t firstCommand,
      std::size_t commandCount);

 private:
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;

  VkFormat m_colorFormat = VK_FORMAT_UNDEFINED;

  std::uint32_t m_framesInFlight = 2;
  std::uint32_t m_maxQuadsPerBatch = 2048;

  VulkanShaderModule m_vertexShader;
  VulkanShaderModule m_fragmentShader;
  VulkanGraphicsPipeline m_pipeline;

  std::vector<VulkanBuffer> m_frameVertexBuffers;
  VulkanBuffer m_indexBuffer;

  std::vector<QuadDrawCommand> m_quadCommands;
};

}  // namespace eng::vk
#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace eng::vk
{

struct VulkanGraphicsPipelineDesc final
{
  VkDevice device = VK_NULL_HANDLE;

  VkShaderModule vertexShader = VK_NULL_HANDLE;
  VkShaderModule fragmentShader = VK_NULL_HANDLE;

  VkFormat colorFormat = VK_FORMAT_UNDEFINED;

  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

  std::vector<VkVertexInputBindingDescription> vertexBindings;
  std::vector<VkVertexInputAttributeDescription> vertexAttributes;

  std::vector<VkPushConstantRange> pushConstantRanges;
};

class VulkanGraphicsPipeline final
{
 public:
  explicit VulkanGraphicsPipeline(const VulkanGraphicsPipelineDesc& desc);
  ~VulkanGraphicsPipeline();

  VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
  VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

  VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept;
  VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&& other) noexcept;

  [[nodiscard]] VkPipeline Get() const;
  [[nodiscard]] VkPipelineLayout Layout() const;

 private:
  void Create(const VulkanGraphicsPipelineDesc& desc);
  void Destroy();

 private:
  VkDevice m_device = VK_NULL_HANDLE;

  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;
};

}  // namespace eng::vk
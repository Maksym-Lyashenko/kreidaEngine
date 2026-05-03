#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace eng::vk
{

struct VulkanDescriptorSetLayoutDesc final
{
  VkDevice device = VK_NULL_HANDLE;
  std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class VulkanDescriptorSetLayout final
{
 public:
  VulkanDescriptorSetLayout() = default;
  explicit VulkanDescriptorSetLayout(const VulkanDescriptorSetLayoutDesc& desc);
  ~VulkanDescriptorSetLayout();

  VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
  VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

  VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept;
  VulkanDescriptorSetLayout& operator=(VulkanDescriptorSetLayout&& other) noexcept;

  [[nodiscard]] VkDescriptorSetLayout Get() const;

 private:
  void Create(const VulkanDescriptorSetLayoutDesc& desc);
  void Destroy();

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
};

}  // namespace eng::vk
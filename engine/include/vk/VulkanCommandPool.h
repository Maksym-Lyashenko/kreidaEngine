#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace eng::vk
{

struct VulkanCommandPoolDesc final
{
  VkDevice device = VK_NULL_HANDLE;
  std::uint32_t queueFamilyIndex = UINT32_MAX;

  VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
};

class VulkanCommandPool final
{
 public:
  explicit VulkanCommandPool(const VulkanCommandPoolDesc& desc);
  ~VulkanCommandPool();

  VulkanCommandPool(const VulkanCommandPool&) = delete;
  VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;

  VulkanCommandPool(VulkanCommandPool&& other) noexcept;
  VulkanCommandPool& operator=(VulkanCommandPool&& other) noexcept;

  [[nodiscard]] VkCommandPool Get() const;
  [[nodiscard]] VkDevice Device() const;
  [[nodiscard]] std::uint32_t QueueFamilyIndex() const;

  void Reset(VkCommandPoolResetFlags flags = 0);

 private:
  void Create(const VulkanCommandPoolDesc& desc);
  void Destroy();

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkCommandPool m_commandPool = VK_NULL_HANDLE;
  std::uint32_t m_queueFamilyIndex = UINT32_MAX;
};

}  // namespace eng::vk
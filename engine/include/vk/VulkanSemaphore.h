#pragma once

#include <vulkan/vulkan.h>

namespace eng::vk
{

struct VulkanSemaphoreDesc final
{
  VkDevice device = VK_NULL_HANDLE;
};

class VulkanSemaphore final
{
 public:
  VulkanSemaphore() = default;
  explicit VulkanSemaphore(const VulkanSemaphoreDesc& desc);
  ~VulkanSemaphore();

  VulkanSemaphore(const VulkanSemaphore&) = delete;
  VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

  VulkanSemaphore(VulkanSemaphore&& other) noexcept;
  VulkanSemaphore& operator=(VulkanSemaphore&& other) noexcept;

  [[nodiscard]] VkSemaphore Get() const;

 private:
  void Create(const VulkanSemaphoreDesc& desc);
  void Destroy();

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkSemaphore m_semaphore = VK_NULL_HANDLE;
};

}  // namespace eng::vk
#pragma once

#include <vulkan/vulkan.h>

namespace eng::vk
{

struct VulkanFrameSyncDesc final
{
  VkDevice device = VK_NULL_HANDLE;
  bool createFenceSignaled = true;
};

class VulkanFrameSync final
{
 public:
  VulkanFrameSync() = default;
  explicit VulkanFrameSync(const VulkanFrameSyncDesc& desc);
  ~VulkanFrameSync();

  VulkanFrameSync(const VulkanFrameSync&) = delete;
  VulkanFrameSync& operator=(const VulkanFrameSync&) = delete;

  VulkanFrameSync(VulkanFrameSync&& other) noexcept;
  VulkanFrameSync& operator=(VulkanFrameSync&& other) noexcept;

  [[nodiscard]] VkSemaphore ImageAvailableSemaphore() const;
  [[nodiscard]] VkFence InFlightFence() const;

  void WaitFence() const;
  void ResetFence() const;

 private:
  void Create(const VulkanFrameSyncDesc& desc);
  void Destroy();

 private:
  VkDevice m_device = VK_NULL_HANDLE;

  VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
  VkFence m_inFlightFence = VK_NULL_HANDLE;
};

}  // namespace eng::vk
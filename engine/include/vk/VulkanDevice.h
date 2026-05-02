#pragma once

#include "VulkanPhysicalDevice.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace eng::vk
{

struct VulkanDeviceDesc final
{
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  QueueFamilyIndices queueFamilies{};

  bool enableDynamicRendering = true;
  bool enableSynchronization2 = true;

  std::vector<const char*> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

class VulkanDevice final
{
 public:
  explicit VulkanDevice(const VulkanDeviceDesc& desc);
  ~VulkanDevice();

  VulkanDevice(const VulkanDevice&) = delete;
  VulkanDevice& operator=(const VulkanDevice&) = delete;

  VulkanDevice(VulkanDevice&& other) noexcept;
  VulkanDevice& operator=(VulkanDevice&& other) noexcept;

  [[nodiscard]] VkDevice Get() const;

  [[nodiscard]] VkQueue GraphicsQueue() const;
  [[nodiscard]] VkQueue PresentQueue() const;

  [[nodiscard]] std::uint32_t GraphicsQueueFamily() const;
  [[nodiscard]] std::uint32_t PresentQueueFamily() const;

  void WaitIdle() const;

 private:
  void CreateDevice(const VulkanDeviceDesc& desc);
  void Destroy();

 private:
  VkDevice m_device = VK_NULL_HANDLE;

  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  VkQueue m_presentQueue = VK_NULL_HANDLE;

  std::uint32_t m_graphicsQueueFamily = UINT32_MAX;
  std::uint32_t m_presentQueueFamily = UINT32_MAX;
};

}  // namespace eng::vk
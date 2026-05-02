#pragma once

#include <vulkan/vulkan.h>

namespace eng::vk
{

class VulkanSurface final
{
 public:
  VulkanSurface() = default;
  VulkanSurface(VkInstance instance, VkSurfaceKHR surface);
  ~VulkanSurface();

  VulkanSurface(const VulkanSurface&) = delete;
  VulkanSurface& operator=(const VulkanSurface&) = delete;

  VulkanSurface(VulkanSurface&& other) noexcept;
  VulkanSurface& operator=(VulkanSurface&& other) noexcept;

  [[nodiscard]] VkSurfaceKHR Get() const;
  [[nodiscard]] VkInstance Instance() const;
  [[nodiscard]] bool IsValid() const;

 private:
  void Destroy();

 private:
  VkInstance m_instance = VK_NULL_HANDLE;
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;
};

}  // namespace eng::vk
#pragma once

#include "vk/VulkanPhysicalDevice.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

struct SDL_Window;

namespace eng::vk
{

struct VulkanSwapchainDesc final
{
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  SDL_Window* window = nullptr;

  QueueFamilyIndices queueFamilies{};

  bool vsync = true;
  bool preferMailboxPresentMode = false;

  VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
  VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
};

class VulkanSwapchain final
{
 public:
  explicit VulkanSwapchain(const VulkanSwapchainDesc& desc);
  ~VulkanSwapchain();

  VulkanSwapchain(const VulkanSwapchain&) = delete;
  VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

  VulkanSwapchain(VulkanSwapchain&& other) noexcept;
  VulkanSwapchain& operator=(VulkanSwapchain&& other) noexcept;

  void Recreate(const VulkanSwapchainDesc& desc);

  [[nodiscard]] VkSwapchainKHR Get() const;

  [[nodiscard]] VkFormat ImageFormat() const;
  [[nodiscard]] VkColorSpaceKHR ColorSpace() const;
  [[nodiscard]] VkExtent2D Extent() const;

  [[nodiscard]] const std::vector<VkImage>& Images() const;
  [[nodiscard]] const std::vector<VkImageView>& ImageViews() const;

  [[nodiscard]] std::uint32_t ImageCount() const;

 private:
  void Create(const VulkanSwapchainDesc& desc, VkSwapchainKHR oldSwapchain);
  void Destroy();

  void DestroyImageViews();

 private:
  VkDevice m_device = VK_NULL_HANDLE;

  VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

  VkFormat m_imageFormat = VK_FORMAT_UNDEFINED;
  VkColorSpaceKHR m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  VkExtent2D m_extent{};

  std::vector<VkImage> m_images;
  std::vector<VkImageView> m_imageViews;
};

}  // namespace eng::vk
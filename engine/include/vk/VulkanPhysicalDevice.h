#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace eng::vk
{

struct QueueFamilyIndices final
{
  std::optional<std::uint32_t> graphicsFamily;
  std::optional<std::uint32_t> presentFamily;

  [[nodiscard]] bool IsComplete() const
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }

  [[nodiscard]] bool HasSeparatePresentQueue() const
  {
    return IsComplete() && graphicsFamily.value() != presentFamily.value();
  }
};

struct SwapchainSupportDetails final
{
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;

  [[nodiscard]] bool IsAdequate() const
  {
    return !formats.empty() && !presentModes.empty();
  }
};

struct VulkanPhysicalDeviceDesc final
{
  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  std::uint32_t targetApiVersion = VK_API_VERSION_1_3;
  bool preferDiscreteGpu = true;

  bool requireDynamicRendering = true;
  bool requireSynchronization2 = true;

  std::vector<const char*> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

class VulkanPhysicalDevice final
{
 public:
  explicit VulkanPhysicalDevice(const VulkanPhysicalDeviceDesc& desc);
  ~VulkanPhysicalDevice() = default;

  VulkanPhysicalDevice(const VulkanPhysicalDevice&) = delete;
  VulkanPhysicalDevice& operator=(const VulkanPhysicalDevice&) = delete;

  VulkanPhysicalDevice(VulkanPhysicalDevice&&) noexcept = default;
  VulkanPhysicalDevice& operator=(VulkanPhysicalDevice&&) noexcept = default;

  [[nodiscard]] VkPhysicalDevice Get() const;

  [[nodiscard]] const VkPhysicalDeviceProperties& Properties() const;
  [[nodiscard]] const VkPhysicalDeviceFeatures& Features() const;

  [[nodiscard]] const QueueFamilyIndices& QueueFamilies() const;
  [[nodiscard]] std::uint32_t GraphicsQueueFamily() const;
  [[nodiscard]] std::uint32_t PresentQueueFamily() const;

  [[nodiscard]] SwapchainSupportDetails QuerySwapchainSupport() const;

 private:
  void PickPhysicalDevice(const VulkanPhysicalDeviceDesc& desc);

 private:
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkSurfaceKHR m_surface = VK_NULL_HANDLE;

  VkPhysicalDeviceProperties m_properties{};
  VkPhysicalDeviceFeatures m_features{};

  QueueFamilyIndices m_queueFamilies{};
};

}  // namespace eng::vk
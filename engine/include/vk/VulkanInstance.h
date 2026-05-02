#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace eng::vk
{

struct VulkanInstanceDesc final
{
  const char* appName = "Kreida Engine";
  const char* engineName = "Kreida Engine";

  std::uint32_t appVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
  std::uint32_t engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);

  std::uint32_t targetApiVersion = VK_API_VERSION_1_3;

  bool enableValidation = true;
  bool enableDebugUtils = true;

  std::vector<const char*> requiredInstanceExtensions;

  std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
};

class VulkanInstance final
{
 public:
  explicit VulkanInstance(const VulkanInstanceDesc& desc);
  ~VulkanInstance();

  VulkanInstance(const VulkanInstance&) = delete;
  VulkanInstance& operator=(const VulkanInstance&) = delete;

  VulkanInstance(VulkanInstance&& other) noexcept;
  VulkanInstance& operator=(VulkanInstance&& other) noexcept;

  [[nodiscard]] VkInstance Get() const;
  [[nodiscard]] std::uint32_t ApiVersion() const;

 private:
  void CreateInstance(const VulkanInstanceDesc& desc);
  void CreateDebugMessenger();
  void Destroy();

 private:
  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

  std::uint32_t m_apiVersion = VK_API_VERSION_1_0;

  bool m_debugUtilsEnabled = false;
};

}  // namespace eng::vk
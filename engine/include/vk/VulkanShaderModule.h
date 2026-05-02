#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>
#include <vector>

namespace eng::vk
{

struct VulkanShaderModuleDesc final
{
  VkDevice device = VK_NULL_HANDLE;
  std::filesystem::path path;
};

class VulkanShaderModule final
{
 public:
  explicit VulkanShaderModule(const VulkanShaderModuleDesc& desc);
  ~VulkanShaderModule();

  VulkanShaderModule(const VulkanShaderModule&) = delete;
  VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;

  VulkanShaderModule(VulkanShaderModule&& other) noexcept;
  VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

  [[nodiscard]] VkShaderModule Get() const;

 private:
  void Create(const VulkanShaderModuleDesc& desc);
  void Destroy();

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkShaderModule m_shaderModule = VK_NULL_HANDLE;
};

}  // namespace eng::vk
#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace eng::vk
{

struct VulkanTexture2DDesc final
{
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkQueue graphicsQueue = VK_NULL_HANDLE;

  std::uint32_t width = 0;
  std::uint32_t height = 0;

  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

  const void* pixels = nullptr;
  VkDeviceSize pixelsSize = 0;
};

class VulkanTexture2D final
{
 public:
  VulkanTexture2D() = default;
  explicit VulkanTexture2D(const VulkanTexture2DDesc& desc);
  ~VulkanTexture2D();

  VulkanTexture2D(const VulkanTexture2D&) = delete;
  VulkanTexture2D& operator=(const VulkanTexture2D&) = delete;

  VulkanTexture2D(VulkanTexture2D&& other) noexcept;
  VulkanTexture2D& operator=(VulkanTexture2D&& other) noexcept;

  [[nodiscard]] VkImage Image() const;
  [[nodiscard]] VkImageView ImageView() const;
  [[nodiscard]] VkSampler Sampler() const;
  [[nodiscard]] VkFormat Format() const;
  [[nodiscard]] VkDescriptorSet DescriptorSet() const;

  void SetDescriptorSet(VkDescriptorSet descriptorSet);

  [[nodiscard]] std::uint32_t Width() const;
  [[nodiscard]] std::uint32_t Height() const;

 private:
  void Create(const VulkanTexture2DDesc& desc);
  void Destroy();

 private:
  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;

  VkImage m_image = VK_NULL_HANDLE;
  VkDeviceMemory m_memory = VK_NULL_HANDLE;
  VkImageView m_imageView = VK_NULL_HANDLE;
  VkSampler m_sampler = VK_NULL_HANDLE;

  VkFormat m_format = VK_FORMAT_UNDEFINED;
  std::uint32_t m_width = 0;
  std::uint32_t m_height = 0;

  VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};

}  // namespace eng::vk
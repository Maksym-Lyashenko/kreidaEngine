#pragma once

#include <vulkan/vulkan.h>

namespace eng::vk
{

struct VulkanBufferDesc final
{
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkDeviceSize size = 0;
  VkBufferUsageFlags usage = 0;
  VkMemoryPropertyFlags memoryProperties = 0;

  const void* initialData = nullptr;
  VkDeviceSize initialDataSize = 0;
};

class VulkanBuffer final
{
 public:
  VulkanBuffer() = default;
  explicit VulkanBuffer(const VulkanBufferDesc& desc);
  ~VulkanBuffer();

  VulkanBuffer(const VulkanBuffer&) = delete;
  VulkanBuffer& operator=(const VulkanBuffer&) = delete;

  VulkanBuffer(VulkanBuffer&& other) noexcept;
  VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

  [[nodiscard]] VkBuffer Get() const;
  [[nodiscard]] VkDeviceSize Size() const;

  void Upload(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

 private:
  void Create(const VulkanBufferDesc& desc);
  void Destroy();

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkBuffer m_buffer = VK_NULL_HANDLE;
  VkDeviceMemory m_memory = VK_NULL_HANDLE;
  VkDeviceSize m_size = 0;
};

}  // namespace eng::vk
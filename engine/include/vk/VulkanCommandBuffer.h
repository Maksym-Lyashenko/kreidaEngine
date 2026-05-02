#pragma once

#include <vulkan/vulkan.h>

namespace eng::vk
{

struct VulkanCommandBufferDesc final
{
  VkDevice device = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
};

class VulkanCommandBuffer final
{
 public:
  VulkanCommandBuffer() = default;
  explicit VulkanCommandBuffer(const VulkanCommandBufferDesc& desc);
  ~VulkanCommandBuffer();

  VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
  VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

  VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept;
  VulkanCommandBuffer& operator=(VulkanCommandBuffer&& other) noexcept;

  [[nodiscard]] VkCommandBuffer Get() const;
  [[nodiscard]] bool IsValid() const;

  void Reset(VkCommandBufferResetFlags flags = 0);
  void Begin(VkCommandBufferUsageFlags usageFlags = 0);
  void BeginOneTimeSubmit();
  void End();

 private:
  void Allocate(const VulkanCommandBufferDesc& desc);
  void Free();

 private:
  VkDevice m_device = VK_NULL_HANDLE;
  VkCommandPool m_commandPool = VK_NULL_HANDLE;
  VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
};

}  // namespace eng::vk
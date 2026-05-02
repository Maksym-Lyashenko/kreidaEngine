#include "vk/VulkanCommandBuffer.h"

#include <stdexcept>

namespace eng::vk
{

namespace
{

void vkCheck(VkResult result, const char* message)
{
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error(message);
  }
}

}  // namespace

VulkanCommandBuffer::VulkanCommandBuffer(const VulkanCommandBufferDesc& desc)
{
  Allocate(desc);
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
  Free();
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept
{
  m_device = other.m_device;
  m_commandPool = other.m_commandPool;
  m_commandBuffer = other.m_commandBuffer;

  other.m_device = VK_NULL_HANDLE;
  other.m_commandPool = VK_NULL_HANDLE;
  other.m_commandBuffer = VK_NULL_HANDLE;
}

VulkanCommandBuffer& VulkanCommandBuffer::operator=(VulkanCommandBuffer&& other) noexcept
{
  if (this != &other)
  {
    Free();

    m_device = other.m_device;
    m_commandPool = other.m_commandPool;
    m_commandBuffer = other.m_commandBuffer;

    other.m_device = VK_NULL_HANDLE;
    other.m_commandPool = VK_NULL_HANDLE;
    other.m_commandBuffer = VK_NULL_HANDLE;
  }

  return *this;
}

VkCommandBuffer VulkanCommandBuffer::Get() const
{
  return m_commandBuffer;
}

bool VulkanCommandBuffer::IsValid() const
{
  return m_device != VK_NULL_HANDLE && m_commandPool != VK_NULL_HANDLE &&
         m_commandBuffer != VK_NULL_HANDLE;
}

void VulkanCommandBuffer::Reset(VkCommandBufferResetFlags flags)
{
  if (m_commandBuffer == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot reset VulkanCommandBuffer: command buffer is null.");
  }

  vkCheck(vkResetCommandBuffer(m_commandBuffer, flags), "Failed to reset Vulkan command buffer.");
}

void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags usageFlags)
{
  if (m_commandBuffer == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot begin VulkanCommandBuffer: command buffer is null.");
  }

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = usageFlags;
  beginInfo.pInheritanceInfo = nullptr;

  vkCheck(
      vkBeginCommandBuffer(m_commandBuffer, &beginInfo), "Failed to begin Vulkan command buffer.");
}

void VulkanCommandBuffer::BeginOneTimeSubmit()
{
  Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void VulkanCommandBuffer::End()
{
  if (m_commandBuffer == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot end VulkanCommandBuffer: command buffer is null.");
  }

  vkCheck(vkEndCommandBuffer(m_commandBuffer), "Failed to end Vulkan command buffer.");
}

void VulkanCommandBuffer::Allocate(const VulkanCommandBufferDesc& desc)
{
  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot allocate VulkanCommandBuffer: device is null.");
  }

  if (desc.commandPool == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot allocate VulkanCommandBuffer: command pool is null.");
  }

  m_device = desc.device;
  m_commandPool = desc.commandPool;

  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = m_commandPool;
  allocateInfo.level = desc.level;
  allocateInfo.commandBufferCount = 1;

  vkCheck(
      vkAllocateCommandBuffers(m_device, &allocateInfo, &m_commandBuffer),
      "Failed to allocate Vulkan command buffer.");
}

void VulkanCommandBuffer::Free()
{
  if (m_commandBuffer != VK_NULL_HANDLE)
  {
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_commandBuffer);
    m_commandBuffer = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;
  m_commandPool = VK_NULL_HANDLE;
}

}  // namespace eng::vk
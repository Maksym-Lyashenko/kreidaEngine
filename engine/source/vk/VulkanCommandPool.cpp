#include "vk/VulkanCommandPool.h"

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

VulkanCommandPool::VulkanCommandPool(const VulkanCommandPoolDesc& desc)
{
  Create(desc);
}

VulkanCommandPool::~VulkanCommandPool()
{
  Destroy();
}

VulkanCommandPool::VulkanCommandPool(VulkanCommandPool&& other) noexcept
{
  m_device = other.m_device;
  m_commandPool = other.m_commandPool;
  m_queueFamilyIndex = other.m_queueFamilyIndex;

  other.m_device = VK_NULL_HANDLE;
  other.m_commandPool = VK_NULL_HANDLE;
  other.m_queueFamilyIndex = UINT32_MAX;
}

VulkanCommandPool& VulkanCommandPool::operator=(VulkanCommandPool&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_commandPool = other.m_commandPool;
    m_queueFamilyIndex = other.m_queueFamilyIndex;

    other.m_device = VK_NULL_HANDLE;
    other.m_commandPool = VK_NULL_HANDLE;
    other.m_queueFamilyIndex = UINT32_MAX;
  }

  return *this;
}

VkCommandPool VulkanCommandPool::Get() const
{
  return m_commandPool;
}

VkDevice VulkanCommandPool::Device() const
{
  return m_device;
}

std::uint32_t VulkanCommandPool::QueueFamilyIndex() const
{
  return m_queueFamilyIndex;
}

void VulkanCommandPool::Reset(VkCommandPoolResetFlags flags)
{
  if (m_commandPool == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot reset VulkanCommandPool: command pool is null.");
  }

  vkCheck(
      vkResetCommandPool(m_device, m_commandPool, flags), "Failed to reset Vulkan command pool.");
}

void VulkanCommandPool::Create(const VulkanCommandPoolDesc& desc)
{
  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanCommandPool: device is null.");
  }

  if (desc.queueFamilyIndex == UINT32_MAX)
  {
    throw std::runtime_error("Cannot create VulkanCommandPool: queue family index is invalid.");
  }

  m_device = desc.device;
  m_queueFamilyIndex = desc.queueFamilyIndex;

  VkCommandPoolCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  createInfo.flags = desc.flags;
  createInfo.queueFamilyIndex = desc.queueFamilyIndex;

  vkCheck(
      vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool),
      "Failed to create Vulkan command pool.");
}

void VulkanCommandPool::Destroy()
{
  if (m_commandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;
  m_queueFamilyIndex = UINT32_MAX;
}

}  // namespace eng::vk
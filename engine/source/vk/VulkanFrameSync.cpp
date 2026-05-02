#include "vk/VulkanFrameSync.h"

#include <cstdint>
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

VulkanFrameSync::VulkanFrameSync(const VulkanFrameSyncDesc& desc)
{
  Create(desc);
}

VulkanFrameSync::~VulkanFrameSync()
{
  Destroy();
}

VulkanFrameSync::VulkanFrameSync(VulkanFrameSync&& other) noexcept
{
  m_device = other.m_device;
  m_imageAvailableSemaphore = other.m_imageAvailableSemaphore;
  m_inFlightFence = other.m_inFlightFence;

  other.m_device = VK_NULL_HANDLE;
  other.m_imageAvailableSemaphore = VK_NULL_HANDLE;
  other.m_inFlightFence = VK_NULL_HANDLE;
}

VulkanFrameSync& VulkanFrameSync::operator=(VulkanFrameSync&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_imageAvailableSemaphore = other.m_imageAvailableSemaphore;
    m_inFlightFence = other.m_inFlightFence;

    other.m_device = VK_NULL_HANDLE;
    other.m_imageAvailableSemaphore = VK_NULL_HANDLE;
    other.m_inFlightFence = VK_NULL_HANDLE;
  }

  return *this;
}

VkSemaphore VulkanFrameSync::ImageAvailableSemaphore() const
{
  return m_imageAvailableSemaphore;
}

VkFence VulkanFrameSync::InFlightFence() const
{
  return m_inFlightFence;
}

void VulkanFrameSync::WaitFence() const
{
  if (m_device == VK_NULL_HANDLE || m_inFlightFence == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot wait VulkanFrameSync fence: fence is null.");
  }

  vkCheck(
      vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX),
      "Failed to wait for Vulkan frame fence.");
}

void VulkanFrameSync::ResetFence() const
{
  if (m_device == VK_NULL_HANDLE || m_inFlightFence == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot reset VulkanFrameSync fence: fence is null.");
  }

  vkCheck(vkResetFences(m_device, 1, &m_inFlightFence), "Failed to reset Vulkan frame fence.");
}

void VulkanFrameSync::Create(const VulkanFrameSyncDesc& desc)
{
  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanFrameSync: device is null.");
  }

  m_device = desc.device;

  VkSemaphoreCreateInfo semaphoreCreateInfo{};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  vkCheck(
      vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphore),
      "Failed to create Vulkan image available semaphore.");

  VkFenceCreateInfo fenceCreateInfo{};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  if (desc.createFenceSignaled)
  {
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  }

  vkCheck(
      vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_inFlightFence),
      "Failed to create Vulkan in-flight fence.");
}

void VulkanFrameSync::Destroy()
{
  if (m_inFlightFence != VK_NULL_HANDLE)
  {
    vkDestroyFence(m_device, m_inFlightFence, nullptr);
    m_inFlightFence = VK_NULL_HANDLE;
  }

  if (m_imageAvailableSemaphore != VK_NULL_HANDLE)
  {
    vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
    m_imageAvailableSemaphore = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;
}

}  // namespace eng::vk
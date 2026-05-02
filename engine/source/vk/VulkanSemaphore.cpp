#include "vk/VulkanSemaphore.h"

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

VulkanSemaphore::VulkanSemaphore(const VulkanSemaphoreDesc& desc)
{
  Create(desc);
}

VulkanSemaphore::~VulkanSemaphore()
{
  Destroy();
}

VulkanSemaphore::VulkanSemaphore(VulkanSemaphore&& other) noexcept
{
  m_device = other.m_device;
  m_semaphore = other.m_semaphore;

  other.m_device = VK_NULL_HANDLE;
  other.m_semaphore = VK_NULL_HANDLE;
}

VulkanSemaphore& VulkanSemaphore::operator=(VulkanSemaphore&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_semaphore = other.m_semaphore;

    other.m_device = VK_NULL_HANDLE;
    other.m_semaphore = VK_NULL_HANDLE;
  }

  return *this;
}

VkSemaphore VulkanSemaphore::Get() const
{
  return m_semaphore;
}

void VulkanSemaphore::Create(const VulkanSemaphoreDesc& desc)
{
  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanSemaphore: device is null.");
  }

  m_device = desc.device;

  VkSemaphoreCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  vkCheck(
      vkCreateSemaphore(m_device, &createInfo, nullptr, &m_semaphore),
      "Failed to create Vulkan semaphore.");
}

void VulkanSemaphore::Destroy()
{
  if (m_semaphore != VK_NULL_HANDLE)
  {
    vkDestroySemaphore(m_device, m_semaphore, nullptr);
    m_semaphore = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;
}

}  // namespace eng::vk
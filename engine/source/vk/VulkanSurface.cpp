#include "vk/VulkanSurface.h"

#include <SDL3/SDL_vulkan.h>

namespace eng::vk
{

VulkanSurface::VulkanSurface(VkInstance instance, VkSurfaceKHR surface)
    : m_instance(instance), m_surface(surface)
{
}

VulkanSurface::~VulkanSurface()
{
  Destroy();
}

VulkanSurface::VulkanSurface(VulkanSurface&& other) noexcept
{
  m_instance = other.m_instance;
  m_surface = other.m_surface;

  other.m_instance = VK_NULL_HANDLE;
  other.m_surface = VK_NULL_HANDLE;
}

VulkanSurface& VulkanSurface::operator=(VulkanSurface&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_instance = other.m_instance;
    m_surface = other.m_surface;

    other.m_instance = VK_NULL_HANDLE;
    other.m_surface = VK_NULL_HANDLE;
  }

  return *this;
}

VkSurfaceKHR VulkanSurface::Get() const
{
  return m_surface;
}

VkInstance VulkanSurface::Instance() const
{
  return m_instance;
}

bool VulkanSurface::IsValid() const
{
  return m_instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE;
}

void VulkanSurface::Destroy()
{
  if (m_surface != VK_NULL_HANDLE)
  {
    SDL_Vulkan_DestroySurface(m_instance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
  }

  m_instance = VK_NULL_HANDLE;
}

}  // namespace eng::vk
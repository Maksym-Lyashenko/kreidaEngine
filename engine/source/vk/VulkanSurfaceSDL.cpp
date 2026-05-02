#include "vk/VulkanSurfaceSDL.h"

#include <stdexcept>
#include <string>

namespace eng::vk
{

std::vector<const char*> GetRequiredSDLVulkanInstanceExtensions()
{
  Uint32 extensionCount = 0;

  const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

  if (extensions == nullptr || extensionCount == 0)
  {
    throw std::runtime_error(
        std::string("SDL_Vulkan_GetInstanceExtensions failed: ") + SDL_GetError());
  }

  std::vector<const char*> result;
  result.reserve(extensionCount);

  for (Uint32 i = 0; i < extensionCount; ++i)
  {
    result.push_back(extensions[i]);
  }

  return result;
}

VulkanSurface CreateSDLVulkanSurface(VkInstance instance, SDL_Window* window)
{
  if (instance == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create SDL Vulkan surface: VkInstance is null.");
  }

  if (window == nullptr)
  {
    throw std::runtime_error("Cannot create SDL Vulkan surface: SDL_Window is null.");
  }

  VkSurfaceKHR surface = VK_NULL_HANDLE;

  const bool created = SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);

  if (!created || surface == VK_NULL_HANDLE)
  {
    throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface failed: ") + SDL_GetError());
  }

  return VulkanSurface(instance, surface);
}

}  // namespace eng::vk
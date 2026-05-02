#pragma once

#include "vk/VulkanSurface.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vector>

namespace eng::vk
{

[[nodiscard]] std::vector<const char*> GetRequiredSDLVulkanInstanceExtensions();

[[nodiscard]] VulkanSurface CreateSDLVulkanSurface(VkInstance instance, SDL_Window* window);

}  // namespace eng::vk
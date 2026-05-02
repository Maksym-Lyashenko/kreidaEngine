#include "vk/VulkanSwapchain.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

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

SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
  SwapchainSupportDetails details{};

  vkCheck(
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities),
      "Failed to query Vulkan surface capabilities.");

  std::uint32_t formatCount = 0;

  vkCheck(
      vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr),
      "Failed to query Vulkan surface format count.");

  if (formatCount > 0)
  {
    details.formats.resize(formatCount);

    vkCheck(
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &formatCount, details.formats.data()),
        "Failed to query Vulkan surface formats.");
  }

  std::uint32_t presentModeCount = 0;

  vkCheck(
      vkGetPhysicalDeviceSurfacePresentModesKHR(
          physicalDevice, surface, &presentModeCount, nullptr),
      "Failed to query Vulkan surface present mode count.");

  if (presentModeCount > 0)
  {
    details.presentModes.resize(presentModeCount);

    vkCheck(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surface, &presentModeCount, details.presentModes.data()),
        "Failed to query Vulkan surface present modes.");
  }

  return details;
}

VkSurfaceFormatKHR ChooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats,
    VkFormat preferredFormat,
    VkColorSpaceKHR preferredColorSpace)
{
  for (const VkSurfaceFormatKHR& format : availableFormats)
  {
    if (format.format == preferredFormat && format.colorSpace == preferredColorSpace)
    {
      return format;
    }
  }

  for (const VkSurfaceFormatKHR& format : availableFormats)
  {
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }

  return availableFormats.front();
}

VkPresentModeKHR ChoosePresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes,
    bool vsync,
    bool preferMailboxPresentMode)
{
  if (vsync)
  {
    if (preferMailboxPresentMode)
    {
      for (VkPresentModeKHR presentMode : availablePresentModes)
      {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
          return VK_PRESENT_MODE_MAILBOX_KHR;
        }
      }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  for (VkPresentModeKHR presentMode : availablePresentModes)
  {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }

  for (VkPresentModeKHR presentMode : availablePresentModes)
  {
    if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
    {
      return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GetWindowPixelExtent(SDL_Window* window)
{
  if (window == nullptr)
  {
    throw std::runtime_error("Cannot query swapchain extent: SDL_Window is null.");
  }

  int width = 0;
  int height = 0;

  if (!SDL_GetWindowSizeInPixels(window, &width, &height))
  {
    throw std::runtime_error(std::string("SDL_GetWindowSizeInPixels failed: ") + SDL_GetError());
  }

  if (width <= 0 || height <= 0)
  {
    throw std::runtime_error("Cannot create swapchain: window pixel size is zero.");
  }

  VkExtent2D extent{};
  extent.width = static_cast<std::uint32_t>(width);
  extent.height = static_cast<std::uint32_t>(height);

  return extent;
}

VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window)
{
  if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
  {
    return capabilities.currentExtent;
  }

  VkExtent2D actualExtent = GetWindowPixelExtent(window);

  actualExtent.width = std::clamp(
      actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);

  actualExtent.height = std::clamp(
      actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

  return actualExtent;
}

std::uint32_t ChooseImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
  std::uint32_t imageCount = capabilities.minImageCount + 1;

  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
  {
    imageCount = capabilities.maxImageCount;
  }

  return imageCount;
}

VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format)
{
  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = format;

  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;

  VkImageView imageView = VK_NULL_HANDLE;

  vkCheck(
      vkCreateImageView(device, &createInfo, nullptr, &imageView),
      "Failed to create Vulkan swapchain image view.");

  return imageView;
}

}  // namespace

VulkanSwapchain::VulkanSwapchain(const VulkanSwapchainDesc& desc)
{
  Create(desc, VK_NULL_HANDLE);
}

VulkanSwapchain::~VulkanSwapchain()
{
  Destroy();
}

VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& other) noexcept
{
  m_device = other.m_device;
  m_swapchain = other.m_swapchain;
  m_imageFormat = other.m_imageFormat;
  m_colorSpace = other.m_colorSpace;
  m_extent = other.m_extent;
  m_images = std::move(other.m_images);
  m_imageViews = std::move(other.m_imageViews);

  other.m_device = VK_NULL_HANDLE;
  other.m_swapchain = VK_NULL_HANDLE;
  other.m_imageFormat = VK_FORMAT_UNDEFINED;
  other.m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  other.m_extent = {};
}

VulkanSwapchain& VulkanSwapchain::operator=(VulkanSwapchain&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_swapchain = other.m_swapchain;
    m_imageFormat = other.m_imageFormat;
    m_colorSpace = other.m_colorSpace;
    m_extent = other.m_extent;
    m_images = std::move(other.m_images);
    m_imageViews = std::move(other.m_imageViews);

    other.m_device = VK_NULL_HANDLE;
    other.m_swapchain = VK_NULL_HANDLE;
    other.m_imageFormat = VK_FORMAT_UNDEFINED;
    other.m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    other.m_extent = {};
  }

  return *this;
}

void VulkanSwapchain::Recreate(const VulkanSwapchainDesc& desc)
{
  DestroyImageViews();

  VkSwapchainKHR oldSwapchain = m_swapchain;
  m_swapchain = VK_NULL_HANDLE;

  Create(desc, oldSwapchain);

  if (oldSwapchain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
  }
}

VkSwapchainKHR VulkanSwapchain::Get() const
{
  return m_swapchain;
}

VkFormat VulkanSwapchain::ImageFormat() const
{
  return m_imageFormat;
}

VkColorSpaceKHR VulkanSwapchain::ColorSpace() const
{
  return m_colorSpace;
}

VkExtent2D VulkanSwapchain::Extent() const
{
  return m_extent;
}

const std::vector<VkImage>& VulkanSwapchain::Images() const
{
  return m_images;
}

const std::vector<VkImageView>& VulkanSwapchain::ImageViews() const
{
  return m_imageViews;
}

std::uint32_t VulkanSwapchain::ImageCount() const
{
  return static_cast<std::uint32_t>(m_images.size());
}

void VulkanSwapchain::Create(const VulkanSwapchainDesc& desc, VkSwapchainKHR oldSwapchain)
{
  if (desc.physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanSwapchain: physical device is null.");
  }

  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanSwapchain: device is null.");
  }

  if (desc.surface == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanSwapchain: surface is null.");
  }

  if (desc.window == nullptr)
  {
    throw std::runtime_error("Cannot create VulkanSwapchain: SDL_Window is null.");
  }

  if (!desc.queueFamilies.IsComplete())
  {
    throw std::runtime_error("Cannot create VulkanSwapchain: queue families are incomplete.");
  }

  m_device = desc.device;

  const SwapchainSupportDetails support = QuerySwapchainSupport(desc.physicalDevice, desc.surface);

  if (!support.IsAdequate())
  {
    throw std::runtime_error("Cannot create VulkanSwapchain: swapchain support is not adequate.");
  }

  const VkSurfaceFormatKHR surfaceFormat =
      ChooseSurfaceFormat(support.formats, desc.preferredFormat, desc.preferredColorSpace);

  const VkPresentModeKHR presentMode =
      ChoosePresentMode(support.presentModes, desc.vsync, desc.preferMailboxPresentMode);

  const VkExtent2D extent = ChooseExtent(support.capabilities, desc.window);

  if (extent.width == 0 || extent.height == 0)
  {
    throw std::runtime_error("Cannot create VulkanSwapchain: extent is zero.");
  }

  const std::uint32_t imageCount = ChooseImageCount(support.capabilities);

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = desc.surface;

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  const std::uint32_t queueFamilyIndices[] = {
      desc.queueFamilies.graphicsFamily.value(), desc.queueFamilies.presentFamily.value()};

  if (desc.queueFamilies.HasSeparatePresentQueue())
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform = support.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = oldSwapchain;

  vkCheck(
      vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain),
      "Failed to create Vulkan swapchain.");

  m_imageFormat = surfaceFormat.format;
  m_colorSpace = surfaceFormat.colorSpace;
  m_extent = extent;

  std::uint32_t swapchainImageCount = 0;

  vkCheck(
      vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr),
      "Failed to query Vulkan swapchain image count.");

  m_images.resize(swapchainImageCount);

  vkCheck(
      vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, m_images.data()),
      "Failed to query Vulkan swapchain images.");

  m_imageViews.reserve(m_images.size());

  for (VkImage image : m_images)
  {
    m_imageViews.push_back(CreateImageView(m_device, image, m_imageFormat));
  }
}

void VulkanSwapchain::Destroy()
{
  DestroyImageViews();

  if (m_swapchain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;

  m_imageFormat = VK_FORMAT_UNDEFINED;
  m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  m_extent = {};

  m_images.clear();
}

void VulkanSwapchain::DestroyImageViews()
{
  for (VkImageView imageView : m_imageViews)
  {
    if (imageView != VK_NULL_HANDLE)
    {
      vkDestroyImageView(m_device, imageView, nullptr);
    }
  }

  m_imageViews.clear();
}

}  // namespace eng::vk
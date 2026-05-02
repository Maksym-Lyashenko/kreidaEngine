#include "vk/VulkanPhysicalDevice.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
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

std::vector<VkPhysicalDevice> EnumeratePhysicalDevices(VkInstance instance)
{
  std::uint32_t deviceCount = 0;

  vkCheck(
      vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
      "Failed to enumerate Vulkan physical device count.");

  if (deviceCount == 0)
  {
    throw std::runtime_error("No Vulkan-capable physical devices found.");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);

  vkCheck(
      vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
      "Failed to enumerate Vulkan physical devices.");

  return devices;
}

bool IsDeviceExtensionAvailable(
    const std::vector<VkExtensionProperties>& availableExtensions, const char* requiredExtension)
{
  return std::find_if(
             availableExtensions.begin(),
             availableExtensions.end(),
             [requiredExtension](const VkExtensionProperties& extension)
             {
               return std::strcmp(extension.extensionName, requiredExtension) == 0;
             }) != availableExtensions.end();
}

bool CheckDeviceExtensionSupport(
    VkPhysicalDevice device, const std::vector<const char*>& requiredExtensions)
{
  std::uint32_t extensionCount = 0;

  vkCheck(
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr),
      "Failed to enumerate Vulkan device extension count.");

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);

  vkCheck(
      vkEnumerateDeviceExtensionProperties(
          device, nullptr, &extensionCount, availableExtensions.data()),
      "Failed to enumerate Vulkan device extensions.");

  for (const char* requiredExtension : requiredExtensions)
  {
    if (!IsDeviceExtensionAvailable(availableExtensions, requiredExtension))
    {
      return false;
    }
  }

  return true;
}

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  QueueFamilyIndices indices{};

  std::uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  for (std::uint32_t i = 0; i < queueFamilyCount; ++i)
  {
    const VkQueueFamilyProperties& queueFamily = queueFamilies[i];

    if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
    {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = VK_FALSE;

    vkCheck(
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport),
        "Failed to query Vulkan physical device surface support.");

    if (queueFamily.queueCount > 0 && presentSupport == VK_TRUE)
    {
      indices.presentFamily = i;
    }

    if (indices.IsComplete())
    {
      break;
    }
  }

  return indices;
}

SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  SwapchainSupportDetails details{};

  vkCheck(
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities),
      "Failed to query Vulkan surface capabilities.");

  std::uint32_t formatCount = 0;

  vkCheck(
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr),
      "Failed to query Vulkan surface format count.");

  if (formatCount > 0)
  {
    details.formats.resize(formatCount);

    vkCheck(
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data()),
        "Failed to query Vulkan surface formats.");
  }

  std::uint32_t presentModeCount = 0;

  vkCheck(
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr),
      "Failed to query Vulkan surface present mode count.");

  if (presentModeCount > 0)
  {
    details.presentModes.resize(presentModeCount);

    vkCheck(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details.presentModes.data()),
        "Failed to query Vulkan surface present modes.");
  }

  return details;
}

bool CheckVulkan13FeatureSupport(VkPhysicalDevice device, const VulkanPhysicalDeviceDesc& desc)
{
  VkPhysicalDeviceVulkan13Features vulkan13Features{};
  vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

  VkPhysicalDeviceFeatures2 features2{};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.pNext = &vulkan13Features;

  vkGetPhysicalDeviceFeatures2(device, &features2);

  if (desc.requireDynamicRendering && vulkan13Features.dynamicRendering != VK_TRUE)
  {
    return false;
  }

  if (desc.requireSynchronization2 && vulkan13Features.synchronization2 != VK_TRUE)
  {
    return false;
  }

  return true;
}

bool IsDeviceSuitable(
    VkPhysicalDevice device, VkSurfaceKHR surface, const VulkanPhysicalDeviceDesc& desc)
{
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties(device, &properties);

  if (properties.apiVersion < desc.targetApiVersion)
  {
    return false;
  }

  if (!CheckVulkan13FeatureSupport(device, desc))
  {
    return false;
  }

  const QueueFamilyIndices queueFamilies = FindQueueFamilies(device, surface);

  if (!queueFamilies.IsComplete())
  {
    return false;
  }

  if (!CheckDeviceExtensionSupport(device, desc.requiredDeviceExtensions))
  {
    return false;
  }

  const SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device, surface);

  if (!swapchainSupport.IsAdequate())
  {
    return false;
  }

  return true;
}

std::int32_t ScoreDevice(VkPhysicalDevice device, const VulkanPhysicalDeviceDesc& desc)
{
  VkPhysicalDeviceProperties properties{};
  VkPhysicalDeviceFeatures features{};

  vkGetPhysicalDeviceProperties(device, &properties);
  vkGetPhysicalDeviceFeatures(device, &features);

  if (!IsDeviceSuitable(device, desc.surface, desc))
  {
    return -1;
  }

  std::int32_t score = 0;

  if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
  {
    score += desc.preferDiscreteGpu ? 1000 : 500;
  }
  else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
  {
    score += 500;
  }
  else
  {
    score += 100;
  }

  score += static_cast<std::int32_t>(properties.limits.maxImageDimension2D / 1024);

  return score;
}

}  // namespace

VulkanPhysicalDevice::VulkanPhysicalDevice(const VulkanPhysicalDeviceDesc& desc)
{
  PickPhysicalDevice(desc);
}

VkPhysicalDevice VulkanPhysicalDevice::Get() const
{
  return m_physicalDevice;
}

const VkPhysicalDeviceProperties& VulkanPhysicalDevice::Properties() const
{
  return m_properties;
}

const VkPhysicalDeviceFeatures& VulkanPhysicalDevice::Features() const
{
  return m_features;
}

const QueueFamilyIndices& VulkanPhysicalDevice::QueueFamilies() const
{
  return m_queueFamilies;
}

std::uint32_t VulkanPhysicalDevice::GraphicsQueueFamily() const
{
  return m_queueFamilies.graphicsFamily.value();
}

std::uint32_t VulkanPhysicalDevice::PresentQueueFamily() const
{
  return m_queueFamilies.presentFamily.value();
}

SwapchainSupportDetails VulkanPhysicalDevice::QuerySwapchainSupport() const
{
  if (m_physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot query swapchain support: physical device is null.");
  }

  if (m_surface == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot query swapchain support: surface is null.");
  }

  return eng::vk::QuerySwapchainSupport(m_physicalDevice, m_surface);
}

void VulkanPhysicalDevice::PickPhysicalDevice(const VulkanPhysicalDeviceDesc& desc)
{
  if (desc.instance == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot pick Vulkan physical device: VkInstance is null.");
  }

  if (desc.surface == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot pick Vulkan physical device: VkSurfaceKHR is null.");
  }

  const std::vector<VkPhysicalDevice> devices = EnumeratePhysicalDevices(desc.instance);

  VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
  std::int32_t bestScore = -1;

  for (VkPhysicalDevice device : devices)
  {
    const std::int32_t score = ScoreDevice(device, desc);

    if (score > bestScore)
    {
      bestScore = score;
      bestDevice = device;
    }
  }

  if (bestDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Failed to find a suitable Vulkan physical device for Kreida Engine.");
  }

  m_physicalDevice = bestDevice;
  m_surface = desc.surface;

  vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);
  vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_features);

  m_queueFamilies = FindQueueFamilies(m_physicalDevice, m_surface);

  if (!m_queueFamilies.IsComplete())
  {
    throw std::runtime_error("Selected Vulkan physical device has incomplete queue families.");
  }
}

}  // namespace eng::vk
#include "vk/VulkanDevice.h"

#include <algorithm>
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

bool ContainsQueueFamily(const std::vector<std::uint32_t>& queueFamilies, std::uint32_t queueFamily)
{
  return std::find(queueFamilies.begin(), queueFamilies.end(), queueFamily) != queueFamilies.end();
}

void AppendUniqueQueueFamily(std::vector<std::uint32_t>& queueFamilies, std::uint32_t queueFamily)
{
  if (!ContainsQueueFamily(queueFamilies, queueFamily))
  {
    queueFamilies.push_back(queueFamily);
  }
}

bool IsExtensionAvailable(
    const std::vector<VkExtensionProperties>& extensions, const char* extensionName)
{
  return std::find_if(
             extensions.begin(),
             extensions.end(),
             [extensionName](const VkExtensionProperties& extension)
             {
               return std::strcmp(extension.extensionName, extensionName) == 0;
             }) != extensions.end();
}

void CheckRequiredDeviceExtensions(
    VkPhysicalDevice physicalDevice, const std::vector<const char*>& requiredExtensions)
{
  std::uint32_t extensionCount = 0;

  vkCheck(
      vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr),
      "Failed to enumerate Vulkan device extension count.");

  std::vector<VkExtensionProperties> extensions(extensionCount);

  vkCheck(
      vkEnumerateDeviceExtensionProperties(
          physicalDevice, nullptr, &extensionCount, extensions.data()),
      "Failed to enumerate Vulkan device extensions.");

  for (const char* requiredExtension : requiredExtensions)
  {
    if (!IsExtensionAvailable(extensions, requiredExtension))
    {
      throw std::runtime_error(
          std::string("Required Vulkan device extension is not available: ") + requiredExtension);
    }
  }
}

VkPhysicalDeviceVulkan13Features QueryVulkan13Features(VkPhysicalDevice physicalDevice)
{
  VkPhysicalDeviceVulkan13Features vulkan13Features{};
  vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

  VkPhysicalDeviceFeatures2 features2{};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.pNext = &vulkan13Features;

  vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

  return vulkan13Features;
}

void CheckRequiredVulkan13Features(VkPhysicalDevice physicalDevice, const VulkanDeviceDesc& desc)
{
  const VkPhysicalDeviceVulkan13Features vulkan13Features = QueryVulkan13Features(physicalDevice);

  if (desc.enableDynamicRendering && vulkan13Features.dynamicRendering != VK_TRUE)
  {
    throw std::runtime_error("Vulkan device does not support dynamicRendering.");
  }

  if (desc.enableSynchronization2 && vulkan13Features.synchronization2 != VK_TRUE)
  {
    throw std::runtime_error("Vulkan device does not support synchronization2.");
  }
}

}  // namespace

VulkanDevice::VulkanDevice(const VulkanDeviceDesc& desc)
{
  CreateDevice(desc);
}

VulkanDevice::~VulkanDevice()
{
  Destroy();
}

VulkanDevice::VulkanDevice(VulkanDevice&& other) noexcept
{
  m_device = other.m_device;
  m_graphicsQueue = other.m_graphicsQueue;
  m_presentQueue = other.m_presentQueue;
  m_graphicsQueueFamily = other.m_graphicsQueueFamily;
  m_presentQueueFamily = other.m_presentQueueFamily;

  other.m_device = VK_NULL_HANDLE;
  other.m_graphicsQueue = VK_NULL_HANDLE;
  other.m_presentQueue = VK_NULL_HANDLE;
  other.m_graphicsQueueFamily = UINT32_MAX;
  other.m_presentQueueFamily = UINT32_MAX;
}

VulkanDevice& VulkanDevice::operator=(VulkanDevice&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_graphicsQueue = other.m_graphicsQueue;
    m_presentQueue = other.m_presentQueue;
    m_graphicsQueueFamily = other.m_graphicsQueueFamily;
    m_presentQueueFamily = other.m_presentQueueFamily;

    other.m_device = VK_NULL_HANDLE;
    other.m_graphicsQueue = VK_NULL_HANDLE;
    other.m_presentQueue = VK_NULL_HANDLE;
    other.m_graphicsQueueFamily = UINT32_MAX;
    other.m_presentQueueFamily = UINT32_MAX;
  }

  return *this;
}

VkDevice VulkanDevice::Get() const
{
  return m_device;
}

VkQueue VulkanDevice::GraphicsQueue() const
{
  return m_graphicsQueue;
}

VkQueue VulkanDevice::PresentQueue() const
{
  return m_presentQueue;
}

std::uint32_t VulkanDevice::GraphicsQueueFamily() const
{
  return m_graphicsQueueFamily;
}

std::uint32_t VulkanDevice::PresentQueueFamily() const
{
  return m_presentQueueFamily;
}

void VulkanDevice::WaitIdle() const
{
  if (m_device != VK_NULL_HANDLE)
  {
    vkDeviceWaitIdle(m_device);
  }
}

void VulkanDevice::CreateDevice(const VulkanDeviceDesc& desc)
{
  if (desc.physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanDevice: physical device is null.");
  }

  if (!desc.queueFamilies.IsComplete())
  {
    throw std::runtime_error("Cannot create VulkanDevice: queue families are incomplete.");
  }

  CheckRequiredDeviceExtensions(desc.physicalDevice, desc.requiredDeviceExtensions);
  CheckRequiredVulkan13Features(desc.physicalDevice, desc);

  m_graphicsQueueFamily = desc.queueFamilies.graphicsFamily.value();
  m_presentQueueFamily = desc.queueFamilies.presentFamily.value();

  std::vector<std::uint32_t> uniqueQueueFamilies;
  AppendUniqueQueueFamily(uniqueQueueFamilies, m_graphicsQueueFamily);
  AppendUniqueQueueFamily(uniqueQueueFamilies, m_presentQueueFamily);

  const float queuePriority = 1.0f;

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  queueCreateInfos.reserve(uniqueQueueFamilies.size());

  for (std::uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceVulkan13Features enabledVulkan13Features{};
  enabledVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

  if (desc.enableDynamicRendering)
  {
    enabledVulkan13Features.dynamicRendering = VK_TRUE;
  }

  if (desc.enableSynchronization2)
  {
    enabledVulkan13Features.synchronization2 = VK_TRUE;
  }

  VkPhysicalDeviceFeatures2 enabledFeatures{};
  enabledFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  enabledFeatures.pNext = &enabledVulkan13Features;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.enabledExtensionCount =
      static_cast<std::uint32_t>(desc.requiredDeviceExtensions.size());
  createInfo.ppEnabledExtensionNames =
      desc.requiredDeviceExtensions.empty() ? nullptr : desc.requiredDeviceExtensions.data();

  createInfo.pNext = &enabledFeatures;
  createInfo.pEnabledFeatures = nullptr;

  vkCheck(
      vkCreateDevice(desc.physicalDevice, &createInfo, nullptr, &m_device),
      "Failed to create Vulkan logical device.");

  vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

  vkGetDeviceQueue(m_device, m_presentQueueFamily, 0, &m_presentQueue);
}

void VulkanDevice::Destroy()
{
  if (m_device != VK_NULL_HANDLE)
  {
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
  }

  m_graphicsQueue = VK_NULL_HANDLE;
  m_presentQueue = VK_NULL_HANDLE;

  m_graphicsQueueFamily = UINT32_MAX;
  m_presentQueueFamily = UINT32_MAX;
}

}  // namespace eng::vk
#include "vk/VulkanInstance.h"

#include <algorithm>
#include <cstring>
#include <iostream>
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

bool ContainsName(const std::vector<const char*>& names, const char* name)
{
  return std::find_if(
             names.begin(),
             names.end(),
             [name](const char* item)
             {
               return std::strcmp(item, name) == 0;
             }) != names.end();
}

void AppendUnique(std::vector<const char*>& names, const char* name)
{
  if (!ContainsName(names, name))
  {
    names.push_back(name);
  }
}

std::vector<VkExtensionProperties> EnumerateInstanceExtensions()
{
  std::uint32_t count = 0;

  vkCheck(
      vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr),
      "Failed to enumerate Vulkan instance extension count.");

  std::vector<VkExtensionProperties> extensions(count);

  vkCheck(
      vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()),
      "Failed to enumerate Vulkan instance extensions.");

  return extensions;
}

std::vector<VkLayerProperties> EnumerateInstanceLayers()
{
  std::uint32_t count = 0;

  vkCheck(
      vkEnumerateInstanceLayerProperties(&count, nullptr),
      "Failed to enumerate Vulkan instance layer count.");

  std::vector<VkLayerProperties> layers(count);

  vkCheck(
      vkEnumerateInstanceLayerProperties(&count, layers.data()),
      "Failed to enumerate Vulkan instance layers.");

  return layers;
}

bool IsExtensionAvailable(const std::vector<VkExtensionProperties>& extensions, const char* name)
{
  return std::find_if(
             extensions.begin(),
             extensions.end(),
             [name](const VkExtensionProperties& extension)
             {
               return std::strcmp(extension.extensionName, name) == 0;
             }) != extensions.end();
}

bool IsLayerAvailable(const std::vector<VkLayerProperties>& layers, const char* name)
{
  return std::find_if(
             layers.begin(),
             layers.end(),
             [name](const VkLayerProperties& layer)
             {
               return std::strcmp(layer.layerName, name) == 0;
             }) != layers.end();
}

void CheckRequiredExtensions(const std::vector<const char*>& requiredExtensions)
{
  const auto availableExtensions = EnumerateInstanceExtensions();

  for (const char* requiredExtension : requiredExtensions)
  {
    if (!IsExtensionAvailable(availableExtensions, requiredExtension))
    {
      throw std::runtime_error(
          std::string("Required Vulkan instance extension is not available: ") + requiredExtension);
    }
  }
}

void CheckRequiredLayers(const std::vector<const char*>& requiredLayers)
{
  const auto availableLayers = EnumerateInstanceLayers();

  for (const char* requiredLayer : requiredLayers)
  {
    if (!IsLayerAvailable(availableLayers, requiredLayer))
    {
      throw std::runtime_error(
          std::string("Required Vulkan validation layer is not available: ") + requiredLayer);
    }
  }
}

const char* SeverityToString(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
  switch (severity)
  {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      return "VERBOSE";

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      return "INFO";

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      return "WARNING";

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      return "ERROR";

    default:
      return "UNKNOWN";
  }
}

const char* TypeToString(VkDebugUtilsMessageTypeFlagsEXT type)
{
  if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
  {
    return "VALIDATION";
  }

  if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
  {
    return "PERFORMANCE";
  }

  if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
  {
    return "GENERAL";
  }

  return "UNKNOWN";
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData)
{
  (void)userData;

  std::cerr << "[VK][" << SeverityToString(messageSeverity) << "][" << TypeToString(messageType)
            << "] " << callbackData->pMessage << '\n';

  return VK_FALSE;
}

VkDebugUtilsMessengerCreateInfoEXT MakeDebugMessengerCreateInfo()
{
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           //  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  createInfo.pfnUserCallback = DebugCallback;

  return createInfo;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debugMessenger)
{
  auto function = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

  if (function == nullptr)
  {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  return function(instance, createInfo, allocator, debugMessenger);
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* allocator)
{
  auto function = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

  if (function != nullptr)
  {
    function(instance, debugMessenger, allocator);
  }
}

}  // namespace

VulkanInstance::VulkanInstance(const VulkanInstanceDesc& desc)
{
  CreateInstance(desc);

  if (m_debugUtilsEnabled)
  {
    CreateDebugMessenger();
  }
}

VulkanInstance::~VulkanInstance()
{
  Destroy();
}

VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept
{
  m_instance = other.m_instance;
  m_debugMessenger = other.m_debugMessenger;
  m_apiVersion = other.m_apiVersion;
  m_debugUtilsEnabled = other.m_debugUtilsEnabled;

  other.m_instance = VK_NULL_HANDLE;
  other.m_debugMessenger = VK_NULL_HANDLE;
  other.m_apiVersion = VK_API_VERSION_1_0;
  other.m_debugUtilsEnabled = false;
}

VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_instance = other.m_instance;
    m_debugMessenger = other.m_debugMessenger;
    m_apiVersion = other.m_apiVersion;
    m_debugUtilsEnabled = other.m_debugUtilsEnabled;

    other.m_instance = VK_NULL_HANDLE;
    other.m_debugMessenger = VK_NULL_HANDLE;
    other.m_apiVersion = VK_API_VERSION_1_0;
    other.m_debugUtilsEnabled = false;
  }

  return *this;
}

VkInstance VulkanInstance::Get() const
{
  return m_instance;
}

std::uint32_t VulkanInstance::ApiVersion() const
{
  return m_apiVersion;
}

void VulkanInstance::CreateInstance(const VulkanInstanceDesc& desc)
{
  std::uint32_t supportedApiVersion = VK_API_VERSION_1_0;

  vkCheck(
      vkEnumerateInstanceVersion(&supportedApiVersion),
      "Failed to enumerate Vulkan instance version.");

  if (supportedApiVersion < desc.targetApiVersion)
  {
    throw std::runtime_error("Vulkan 1.3 is not supported by the installed Vulkan loader.");
  }

  m_apiVersion = desc.targetApiVersion;

  std::vector<const char*> extensions = desc.requiredInstanceExtensions;

  if (desc.enableValidation && desc.enableDebugUtils)
  {
    AppendUnique(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    m_debugUtilsEnabled = true;
  }

  CheckRequiredExtensions(extensions);

  std::vector<const char*> layers;

  if (desc.enableValidation)
  {
    layers = desc.validationLayers;
    CheckRequiredLayers(layers);
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = desc.appName;
  appInfo.applicationVersion = desc.appVersion;
  appInfo.pEngineName = desc.engineName;
  appInfo.engineVersion = desc.engineVersion;
  appInfo.apiVersion = desc.targetApiVersion;

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  createInfo.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

  createInfo.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
  createInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();

  if (m_debugUtilsEnabled)
  {
    debugCreateInfo = MakeDebugMessengerCreateInfo();
    createInfo.pNext = &debugCreateInfo;
  }

  vkCheck(vkCreateInstance(&createInfo, nullptr, &m_instance), "Failed to create Vulkan instance.");
}

void VulkanInstance::CreateDebugMessenger()
{
  const auto createInfo = MakeDebugMessengerCreateInfo();

  vkCheck(
      CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger),
      "Failed to create Vulkan debug messenger.");
}

void VulkanInstance::Destroy()
{
  if (m_debugMessenger != VK_NULL_HANDLE)
  {
    DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    m_debugMessenger = VK_NULL_HANDLE;
  }

  if (m_instance != VK_NULL_HANDLE)
  {
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }
}

}  // namespace eng::vk
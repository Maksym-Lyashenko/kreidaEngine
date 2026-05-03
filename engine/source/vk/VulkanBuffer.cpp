#include "vk/VulkanBuffer.h"

#include <cstring>
#include <stdexcept>
#include <cstdint>

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

std::uint32_t FindMemoryType(
    VkPhysicalDevice physicalDevice, std::uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

  for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
  {
    const bool typeMatches = (typeFilter & (1u << i)) != 0;
    const bool propertiesMatch =
        (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;

    if (typeMatches && propertiesMatch)
    {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable Vulkan memory type.");
}

}  // namespace

VulkanBuffer::VulkanBuffer(const VulkanBufferDesc& desc)
{
  Create(desc);
}

VulkanBuffer::~VulkanBuffer()
{
  Destroy();
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
{
  m_device = other.m_device;
  m_buffer = other.m_buffer;
  m_memory = other.m_memory;
  m_size = other.m_size;

  other.m_device = VK_NULL_HANDLE;
  other.m_buffer = VK_NULL_HANDLE;
  other.m_memory = VK_NULL_HANDLE;
  other.m_size = 0;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_buffer = other.m_buffer;
    m_memory = other.m_memory;
    m_size = other.m_size;

    other.m_device = VK_NULL_HANDLE;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_size = 0;
  }

  return *this;
}

VkBuffer VulkanBuffer::Get() const
{
  return m_buffer;
}

VkDeviceSize VulkanBuffer::Size() const
{
  return m_size;
}

void VulkanBuffer::Upload(const void* data, VkDeviceSize size, VkDeviceSize offset)
{
  if (data == nullptr)
  {
    throw std::runtime_error("Cannot upload to VulkanBuffer: data is null.");
  }

  if (m_memory == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot upload to VulkanBuffer: memory is null.");
  }

  if (offset + size > m_size)
  {
    throw std::runtime_error("Cannot upload to VulkanBuffer: upload range exceeds buffer size.");
  }

  void* mappedMemory = nullptr;

  vkCheck(
      vkMapMemory(m_device, m_memory, offset, size, 0, &mappedMemory),
      "Failed to map Vulkan buffer memory.");

  std::memcpy(mappedMemory, data, static_cast<std::size_t>(size));

  vkUnmapMemory(m_device, m_memory);
}

void VulkanBuffer::Create(const VulkanBufferDesc& desc)
{
  if (desc.physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanBuffer: physical device is null.");
  }

  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanBuffer: device is null.");
  }

  if (desc.size == 0)
  {
    throw std::runtime_error("Cannot create VulkanBuffer: size is zero.");
  }

  if (desc.usage == 0)
  {
    throw std::runtime_error("Cannot create VulkanBuffer: usage is zero.");
  }

  if (desc.memoryProperties == 0)
  {
    throw std::runtime_error("Cannot create VulkanBuffer: memory properties are zero.");
  }

  m_device = desc.device;
  m_size = desc.size;

  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.size = desc.size;
  bufferCreateInfo.usage = desc.usage;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  vkCheck(
      vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &m_buffer),
      "Failed to create Vulkan buffer.");

  VkMemoryRequirements memoryRequirements{};
  vkGetBufferMemoryRequirements(m_device, m_buffer, &memoryRequirements);

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = memoryRequirements.size;
  allocateInfo.memoryTypeIndex =
      FindMemoryType(desc.physicalDevice, memoryRequirements.memoryTypeBits, desc.memoryProperties);

  vkCheck(
      vkAllocateMemory(m_device, &allocateInfo, nullptr, &m_memory),
      "Failed to allocate Vulkan buffer memory.");

  vkCheck(
      vkBindBufferMemory(m_device, m_buffer, m_memory, 0), "Failed to bind Vulkan buffer memory.");

  if (desc.initialData != nullptr)
  {
    const VkDeviceSize uploadSize = desc.initialDataSize == 0 ? desc.size : desc.initialDataSize;

    Upload(desc.initialData, uploadSize);
  }
}

void VulkanBuffer::Destroy()
{
  if (m_buffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_device, m_buffer, nullptr);
    m_buffer = VK_NULL_HANDLE;
  }

  if (m_memory != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_device, m_memory, nullptr);
    m_memory = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;
  m_size = 0;
}

}  // namespace eng::vk
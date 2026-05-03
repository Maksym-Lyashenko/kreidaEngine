#include "vk/VulkanTexture2D.h"

#include "vk/VulkanBuffer.h"

#include <cstring>
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

  throw std::runtime_error("Failed to find suitable Vulkan image memory type.");
}

VkCommandBuffer BeginImmediateCommands(VkDevice device, VkCommandPool commandPool)
{
  VkCommandBufferAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.commandPool = commandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

  vkCheck(
      vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer),
      "Failed to allocate immediate Vulkan command buffer.");

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkCheck(
      vkBeginCommandBuffer(commandBuffer, &beginInfo),
      "Failed to begin immediate Vulkan command buffer.");

  return commandBuffer;
}

void EndImmediateCommands(
    VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
  vkCheck(vkEndCommandBuffer(commandBuffer), "Failed to end immediate Vulkan command buffer.");

  VkCommandBufferSubmitInfo commandBufferInfo{};
  commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  commandBufferInfo.commandBuffer = commandBuffer;

  VkSubmitInfo2 submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submitInfo.commandBufferInfoCount = 1;
  submitInfo.pCommandBufferInfos = &commandBufferInfo;

  vkCheck(
      vkQueueSubmit2(queue, 1, &submitInfo, VK_NULL_HANDLE),
      "Failed to submit immediate Vulkan command buffer.");

  vkCheck(vkQueueWaitIdle(queue), "Failed to wait for immediate Vulkan queue.");

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void CmdTransitionTextureImage(
    VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
  VkImageMemoryBarrier2 barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;

  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    barrier.srcAccessMask = 0;

    barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
  }
  else if (
      oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
      newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

    barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
  }
  else
  {
    throw std::runtime_error("Unsupported Vulkan texture layout transition.");
  }

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers = &barrier;

  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void CmdCopyBufferToImage(
    VkCommandBuffer commandBuffer,
    VkBuffer buffer,
    VkImage image,
    std::uint32_t width,
    std::uint32_t height)
{
  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(
      commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format)
{
  VkImageViewCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = format;

  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;

  VkImageView imageView = VK_NULL_HANDLE;

  vkCheck(
      vkCreateImageView(device, &createInfo, nullptr, &imageView),
      "Failed to create Vulkan texture image view.");

  return imageView;
}

VkSampler CreateSampler(VkDevice device)
{
  VkSamplerCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  createInfo.magFilter = VK_FILTER_NEAREST;
  createInfo.minFilter = VK_FILTER_NEAREST;
  createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

  createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  createInfo.mipLodBias = 0.0f;
  createInfo.anisotropyEnable = VK_FALSE;
  createInfo.maxAnisotropy = 1.0f;
  createInfo.compareEnable = VK_FALSE;
  createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

  createInfo.minLod = 0.0f;
  createInfo.maxLod = 0.0f;
  createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
  createInfo.unnormalizedCoordinates = VK_FALSE;

  VkSampler sampler = VK_NULL_HANDLE;

  vkCheck(
      vkCreateSampler(device, &createInfo, nullptr, &sampler),
      "Failed to create Vulkan texture sampler.");

  return sampler;
}

}  // namespace

VulkanTexture2D::VulkanTexture2D(const VulkanTexture2DDesc& desc)
{
  Create(desc);
}

VulkanTexture2D::~VulkanTexture2D()
{
  Destroy();
}

VulkanTexture2D::VulkanTexture2D(VulkanTexture2D&& other) noexcept
{
  m_physicalDevice = other.m_physicalDevice;
  m_device = other.m_device;
  m_image = other.m_image;
  m_memory = other.m_memory;
  m_imageView = other.m_imageView;
  m_sampler = other.m_sampler;
  m_format = other.m_format;
  m_width = other.m_width;
  m_height = other.m_height;
  m_descriptorSet = other.m_descriptorSet;

  other.m_physicalDevice = VK_NULL_HANDLE;
  other.m_device = VK_NULL_HANDLE;
  other.m_image = VK_NULL_HANDLE;
  other.m_memory = VK_NULL_HANDLE;
  other.m_imageView = VK_NULL_HANDLE;
  other.m_sampler = VK_NULL_HANDLE;
  other.m_descriptorSet = VK_NULL_HANDLE;
  other.m_format = VK_FORMAT_UNDEFINED;
  other.m_width = 0;
  other.m_height = 0;
}

VulkanTexture2D& VulkanTexture2D::operator=(VulkanTexture2D&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_physicalDevice = other.m_physicalDevice;
    m_device = other.m_device;
    m_image = other.m_image;
    m_memory = other.m_memory;
    m_imageView = other.m_imageView;
    m_sampler = other.m_sampler;
    m_format = other.m_format;
    m_width = other.m_width;
    m_height = other.m_height;
    m_descriptorSet = other.m_descriptorSet;

    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_device = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_memory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
    other.m_descriptorSet = VK_NULL_HANDLE;
    other.m_format = VK_FORMAT_UNDEFINED;
    other.m_width = 0;
    other.m_height = 0;
  }

  return *this;
}

VkImage VulkanTexture2D::Image() const
{
  return m_image;
}

VkImageView VulkanTexture2D::ImageView() const
{
  return m_imageView;
}

VkSampler VulkanTexture2D::Sampler() const
{
  return m_sampler;
}

VkFormat VulkanTexture2D::Format() const
{
  return m_format;
}

VkDescriptorSet VulkanTexture2D::DescriptorSet() const
{
  return m_descriptorSet;
}

void VulkanTexture2D::SetDescriptorSet(VkDescriptorSet descriptorSet)
{
  m_descriptorSet = descriptorSet;
}

std::uint32_t VulkanTexture2D::Width() const
{
  return m_width;
}

std::uint32_t VulkanTexture2D::Height() const
{
  return m_height;
}

void VulkanTexture2D::Create(const VulkanTexture2DDesc& desc)
{
  if (desc.physicalDevice == VK_NULL_HANDLE || desc.device == VK_NULL_HANDLE ||
      desc.commandPool == VK_NULL_HANDLE || desc.graphicsQueue == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanTexture2D: invalid Vulkan handles.");
  }

  if (desc.width == 0 || desc.height == 0)
  {
    throw std::runtime_error("Cannot create VulkanTexture2D: size is zero.");
  }

  if (desc.pixels == nullptr || desc.pixelsSize == 0)
  {
    throw std::runtime_error("Cannot create VulkanTexture2D: pixels are empty.");
  }

  m_physicalDevice = desc.physicalDevice;
  m_device = desc.device;
  m_format = desc.format;
  m_width = desc.width;
  m_height = desc.height;

  VulkanBufferDesc stagingDesc{};
  stagingDesc.physicalDevice = desc.physicalDevice;
  stagingDesc.device = desc.device;
  stagingDesc.size = desc.pixelsSize;
  stagingDesc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingDesc.memoryProperties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  stagingDesc.initialData = desc.pixels;
  stagingDesc.initialDataSize = desc.pixelsSize;

  VulkanBuffer stagingBuffer(stagingDesc);

  VkImageCreateInfo imageCreateInfo{};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.extent.width = desc.width;
  imageCreateInfo.extent.height = desc.height;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.format = desc.format;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.flags = 0;

  vkCheck(
      vkCreateImage(m_device, &imageCreateInfo, nullptr, &m_image),
      "Failed to create Vulkan texture image.");

  VkMemoryRequirements memoryRequirements{};
  vkGetImageMemoryRequirements(m_device, m_image, &memoryRequirements);

  VkMemoryAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = memoryRequirements.size;
  allocateInfo.memoryTypeIndex = FindMemoryType(
      desc.physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vkCheck(
      vkAllocateMemory(m_device, &allocateInfo, nullptr, &m_memory),
      "Failed to allocate Vulkan texture memory.");

  vkCheck(
      vkBindImageMemory(m_device, m_image, m_memory, 0), "Failed to bind Vulkan texture memory.");

  VkCommandBuffer commandBuffer = BeginImmediateCommands(m_device, desc.commandPool);

  CmdTransitionTextureImage(
      commandBuffer, m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  CmdCopyBufferToImage(commandBuffer, stagingBuffer.Get(), m_image, desc.width, desc.height);

  CmdTransitionTextureImage(
      commandBuffer,
      m_image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  EndImmediateCommands(m_device, desc.commandPool, desc.graphicsQueue, commandBuffer);

  m_imageView = CreateImageView(m_device, m_image, desc.format);
  m_sampler = CreateSampler(m_device);
}

void VulkanTexture2D::Destroy()
{
  if (m_sampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(m_device, m_sampler, nullptr);
    m_sampler = VK_NULL_HANDLE;
  }

  if (m_imageView != VK_NULL_HANDLE)
  {
    vkDestroyImageView(m_device, m_imageView, nullptr);
    m_imageView = VK_NULL_HANDLE;
  }

  if (m_image != VK_NULL_HANDLE)
  {
    vkDestroyImage(m_device, m_image, nullptr);
    m_image = VK_NULL_HANDLE;
  }

  if (m_memory != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_device, m_memory, nullptr);
    m_memory = VK_NULL_HANDLE;
  }

  m_physicalDevice = VK_NULL_HANDLE;
  m_device = VK_NULL_HANDLE;
  m_descriptorSet = VK_NULL_HANDLE;
  m_format = VK_FORMAT_UNDEFINED;
  m_width = 0;
  m_height = 0;
}

}  // namespace eng::vk
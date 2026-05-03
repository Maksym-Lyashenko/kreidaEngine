#include "vk/VulkanDescriptorSetLayout.h"

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

}  // namespace

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const VulkanDescriptorSetLayoutDesc& desc)
{
  Create(desc);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
  Destroy();
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept
{
  m_device = other.m_device;
  m_layout = other.m_layout;

  other.m_device = VK_NULL_HANDLE;
  other.m_layout = VK_NULL_HANDLE;
}

VulkanDescriptorSetLayout& VulkanDescriptorSetLayout::operator=(
    VulkanDescriptorSetLayout&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_layout = other.m_layout;

    other.m_device = VK_NULL_HANDLE;
    other.m_layout = VK_NULL_HANDLE;
  }

  return *this;
}

VkDescriptorSetLayout VulkanDescriptorSetLayout::Get() const
{
  return m_layout;
}

void VulkanDescriptorSetLayout::Create(const VulkanDescriptorSetLayoutDesc& desc)
{
  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanDescriptorSetLayout: device is null.");
  }

  if (desc.bindings.empty())
  {
    throw std::runtime_error("Cannot create VulkanDescriptorSetLayout: bindings are empty.");
  }

  m_device = desc.device;

  VkDescriptorSetLayoutCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  createInfo.bindingCount = static_cast<std::uint32_t>(desc.bindings.size());
  createInfo.pBindings = desc.bindings.data();

  vkCheck(
      vkCreateDescriptorSetLayout(m_device, &createInfo, nullptr, &m_layout),
      "Failed to create Vulkan descriptor set layout.");
}

void VulkanDescriptorSetLayout::Destroy()
{
  if (m_layout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
    m_layout = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;
}

}  // namespace eng::vk
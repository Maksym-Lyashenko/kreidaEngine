#include "vk/VulkanShaderModule.h"

#include <fstream>
#include <stdexcept>
#include <string>

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

std::vector<std::uint32_t> ReadSpirvFile(const std::filesystem::path& path)
{
  std::ifstream file(path, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    throw std::runtime_error("Failed to open SPIR-V file: " + path.string());
  }

  const std::streamsize fileSize = file.tellg();

  if (fileSize <= 0)
  {
    throw std::runtime_error("SPIR-V file is empty: " + path.string());
  }

  if ((fileSize % sizeof(std::uint32_t)) != 0)
  {
    throw std::runtime_error("SPIR-V file size is not aligned to uint32_t: " + path.string());
  }

  std::vector<std::uint32_t> buffer(static_cast<std::size_t>(fileSize) / sizeof(std::uint32_t));

  file.seekg(0);
  file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

  return buffer;
}

}  // namespace

VulkanShaderModule::VulkanShaderModule(const VulkanShaderModuleDesc& desc)
{
  Create(desc);
}

VulkanShaderModule::~VulkanShaderModule()
{
  Destroy();
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule&& other) noexcept
{
  m_device = other.m_device;
  m_shaderModule = other.m_shaderModule;

  other.m_device = VK_NULL_HANDLE;
  other.m_shaderModule = VK_NULL_HANDLE;
}

VulkanShaderModule& VulkanShaderModule::operator=(VulkanShaderModule&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_shaderModule = other.m_shaderModule;

    other.m_device = VK_NULL_HANDLE;
    other.m_shaderModule = VK_NULL_HANDLE;
  }

  return *this;
}

VkShaderModule VulkanShaderModule::Get() const
{
  return m_shaderModule;
}

void VulkanShaderModule::Create(const VulkanShaderModuleDesc& desc)
{
  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanShaderModule: device is null.");
  }

  m_device = desc.device;

  const std::vector<std::uint32_t> code = ReadSpirvFile(desc.path);

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size() * sizeof(std::uint32_t);
  createInfo.pCode = code.data();

  vkCheck(
      vkCreateShaderModule(m_device, &createInfo, nullptr, &m_shaderModule),
      "Failed to create Vulkan shader module.");
}

void VulkanShaderModule::Destroy()
{
  if (m_shaderModule != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
    m_shaderModule = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;
}

}  // namespace eng::vk
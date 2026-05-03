#include "vk/VulkanRenderer2D.h"

#include "vk/VulkanRenderCommands.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace eng::vk
{

namespace
{

struct Renderer2DVertex final
{
  float position[2];
  float uv[2];
  float color[4];
};

struct ProjectionPushConstants final
{
  float projection[16];
};

VkVertexInputBindingDescription Renderer2DVertexBindingDescription()
{
  VkVertexInputBindingDescription desc{};
  desc.binding = 0;
  desc.stride = sizeof(Renderer2DVertex);
  desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return desc;
}

std::array<VkVertexInputAttributeDescription, 3> Renderer2DVertexAttributeDescriptions()
{
  std::array<VkVertexInputAttributeDescription, 3> attributes{};

  attributes[0].location = 0;
  attributes[0].binding = 0;
  attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[0].offset = offsetof(Renderer2DVertex, position);

  attributes[1].location = 1;
  attributes[1].binding = 0;
  attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[1].offset = offsetof(Renderer2DVertex, uv);

  attributes[2].location = 2;
  attributes[2].binding = 0;
  attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[2].offset = offsetof(Renderer2DVertex, color);

  return attributes;
}

ProjectionPushConstants MakeProjectionPushConstants(VkExtent2D extent)
{
  if (extent.width == 0 || extent.height == 0)
  {
    throw std::runtime_error("Cannot create 2D projection: extent is zero.");
  }

  ProjectionPushConstants pc{};

  const float framebufferWidth = static_cast<float>(extent.width);
  const float framebufferHeight = static_cast<float>(extent.height);

  pc.projection[0] = 2.0f / framebufferWidth;
  pc.projection[1] = 0.0f;
  pc.projection[2] = 0.0f;
  pc.projection[3] = 0.0f;

  pc.projection[4] = 0.0f;
  pc.projection[5] = -2.0f / framebufferHeight;
  pc.projection[6] = 0.0f;
  pc.projection[7] = 0.0f;

  pc.projection[8] = 0.0f;
  pc.projection[9] = 0.0f;
  pc.projection[10] = 1.0f;
  pc.projection[11] = 0.0f;

  pc.projection[12] = -1.0f;
  pc.projection[13] = 1.0f;
  pc.projection[14] = 0.0f;
  pc.projection[15] = 1.0f;

  return pc;
}

std::vector<std::uint16_t> BuildQuadIndices(std::uint32_t maxQuads)
{
  std::vector<std::uint16_t> indices;
  indices.resize(static_cast<std::size_t>(maxQuads) * 6);

  for (std::uint32_t quadIndex = 0; quadIndex < maxQuads; ++quadIndex)
  {
    const std::uint16_t vertexOffset = static_cast<std::uint16_t>(quadIndex * 4);

    const std::size_t indexOffset = static_cast<std::size_t>(quadIndex) * 6;

    indices[indexOffset + 0] = vertexOffset + 0;
    indices[indexOffset + 1] = vertexOffset + 1;
    indices[indexOffset + 2] = vertexOffset + 2;

    indices[indexOffset + 3] = vertexOffset + 2;
    indices[indexOffset + 4] = vertexOffset + 3;
    indices[indexOffset + 5] = vertexOffset + 0;
  }

  return indices;
}

void WriteQuadVertices(
    Renderer2DVertex* vertices, std::size_t quadIndex, const QuadDrawCommand& command)
{
  const float x0 = command.x;
  const float y0 = command.y;
  const float x1 = command.x + command.width;
  const float y1 = command.y + command.height;

  const float r = command.color.r;
  const float g = command.color.g;
  const float b = command.color.b;
  const float a = command.color.a;

  const float u0 = command.u0;
  const float v0 = command.v0;
  const float u1 = command.u1;
  const float v1 = command.v1;

  const std::size_t base = quadIndex * 4;

  vertices[base + 0] = Renderer2DVertex{{x0, y0}, {u0, v0}, {r, g, b, a}};
  vertices[base + 1] = Renderer2DVertex{{x1, y0}, {u1, v0}, {r, g, b, a}};
  vertices[base + 2] = Renderer2DVertex{{x1, y1}, {u1, v1}, {r, g, b, a}};
  vertices[base + 3] = Renderer2DVertex{{x0, y1}, {u0, v1}, {r, g, b, a}};
}

}  // namespace

VulkanRenderer2D::VulkanRenderer2D(const VulkanRenderer2DDesc& desc)
    : m_physicalDevice(desc.physicalDevice),
      m_device(desc.device),
      m_commandPool(desc.commandPool),
      m_graphicsQueue(desc.graphicsQueue),
      m_colorFormat(desc.colorFormat),
      m_framesInFlight(desc.framesInFlight),
      m_maxQuadsPerBatch(desc.maxQuadsPerBatch),
      m_vertexShader(MakeShaderModuleDesc(desc.vertexShaderPath)),
      m_fragmentShader(MakeShaderModuleDesc(desc.fragmentShaderPath)),
      m_textureDescriptorSetLayout(MakeTextureDescriptorSetLayoutDesc()),
      m_pipeline(MakePipelineDesc(desc.colorFormat)),
      m_frameVertexBuffers(CreateFrameVertexBuffers()),
      m_indexBuffer(MakeIndexBufferDesc()),
      m_whiteTexture(MakeWhiteTextureDesc())
{
  if (m_physicalDevice == VK_NULL_HANDLE || m_device == VK_NULL_HANDLE ||
      m_commandPool == VK_NULL_HANDLE || m_graphicsQueue == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanRenderer2D: invalid Vulkan handles.");
  }

  if (m_colorFormat == VK_FORMAT_UNDEFINED)
  {
    throw std::runtime_error("Cannot create VulkanRenderer2D: color format is undefined.");
  }

  if (m_framesInFlight == 0)
  {
    throw std::runtime_error("Cannot create VulkanRenderer2D: frames in flight is zero.");
  }

  if (m_maxQuadsPerBatch == 0)
  {
    throw std::runtime_error("Cannot create VulkanRenderer2D: max quads per batch is zero.");
  }

  if (m_maxQuadsPerBatch > 16383)
  {
    throw std::runtime_error(
        "Cannot create VulkanRenderer2D: maxQuadsPerBatch is too large for uint16 indices.");
  }

  UploadIndexBuffer();

  CreateDescriptorPool();

  VkDescriptorSet whiteSet = AllocateTextureDescriptorSet(m_whiteTexture);
  m_whiteTexture.SetDescriptorSet(whiteSet);

  m_quadCommands.reserve(m_maxQuadsPerBatch);
}

VulkanRenderer2D::~VulkanRenderer2D()
{
  DestroyDescriptorPool();
}

std::unique_ptr<VulkanTexture2D> VulkanRenderer2D::CreateTexture2DFromPixels(
    std::uint32_t width, std::uint32_t height, const void* pixels, VkDeviceSize pixelsSize)
{
  VulkanTexture2DDesc desc{};
  desc.physicalDevice = m_physicalDevice;
  desc.device = m_device;
  desc.commandPool = m_commandPool;
  desc.graphicsQueue = m_graphicsQueue;
  desc.width = width;
  desc.height = height;
  desc.format = VK_FORMAT_R8G8B8A8_UNORM;
  desc.pixels = pixels;
  desc.pixelsSize = pixelsSize;

  auto texture = std::make_unique<VulkanTexture2D>(desc);

  VkDescriptorSet descriptorSet = AllocateTextureDescriptorSet(*texture);
  texture->SetDescriptorSet(descriptorSet);

  return texture;
}

void VulkanRenderer2D::OnSwapchainRecreated(VkFormat colorFormat)
{
  if (colorFormat == VK_FORMAT_UNDEFINED)
  {
    throw std::runtime_error("Cannot update VulkanRenderer2D: color format is undefined.");
  }

  if (colorFormat == m_colorFormat)
  {
    return;
  }

  RecreatePipeline(colorFormat);
}

void VulkanRenderer2D::DrawQuad(float x, float y, float width, float height, Color4 color)
{
  if (width <= 0.0f || height <= 0.0f)
  {
    return;
  }

  if (m_quadCommands.size() >= m_maxQuadsPerBatch)
  {
    return;
  }

  QuadDrawCommand command{};
  command.texture = &m_whiteTexture;
  command.x = x;
  command.y = y;
  command.width = width;
  command.height = height;
  command.u0 = 0.0f;
  command.v0 = 0.0f;
  command.u1 = 1.0f;
  command.v1 = 1.0f;
  command.color = color;

  m_quadCommands.push_back(command);
}

void VulkanRenderer2D::DrawTexture(
    const VulkanTexture2D& texture, float x, float y, float width, float height, Color4 tint)
{
  DrawTextureRegion(texture, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, tint);
}

void VulkanRenderer2D::DrawTextureRegion(
    const VulkanTexture2D& texture,
    float x,
    float y,
    float width,
    float height,
    float u0,
    float v0,
    float u1,
    float v1,
    Color4 tint)
{
  if (width <= 0.0f || height <= 0.0f)
  {
    return;
  }

  if (m_quadCommands.size() >= m_maxQuadsPerBatch)
  {
    return;
  }

  if (texture.DescriptorSet() == VK_NULL_HANDLE)
  {
    return;
  }

  QuadDrawCommand command{};
  command.texture = &texture;
  command.x = x;
  command.y = y;
  command.width = width;
  command.height = height;
  command.u0 = u0;
  command.v0 = v0;
  command.u1 = u1;
  command.v1 = v1;
  command.color = tint;

  m_quadCommands.push_back(command);
}

void VulkanRenderer2D::ClearQueuedCommands()
{
  m_quadCommands.clear();
}

void VulkanRenderer2D::RenderQueuedCommands(
    VkCommandBuffer commandBuffer, VkExtent2D extent, std::uint32_t frameIndex)
{
  if (m_quadCommands.empty())
  {
    return;
  }

  if (commandBuffer == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot render queued 2D commands: command buffer is null.");
  }

  if (extent.width == 0 || extent.height == 0)
  {
    throw std::runtime_error("Cannot render queued 2D commands: extent is zero.");
  }

  if (frameIndex >= m_frameVertexBuffers.size())
  {
    throw std::runtime_error("Cannot render queued 2D commands: frame index is invalid.");
  }

  UploadQueuedVertices(frameIndex);

  CmdSetViewportAndScissor(commandBuffer, extent);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.Get());

  const ProjectionPushConstants projection = MakeProjectionPushConstants(extent);

  vkCmdPushConstants(
      commandBuffer,
      m_pipeline.Layout(),
      VK_SHADER_STAGE_VERTEX_BIT,
      0,
      sizeof(ProjectionPushConstants),
      &projection);

  VkBuffer vertexBuffer = m_frameVertexBuffers[frameIndex].Get();
  VkDeviceSize vertexOffset = 0;

  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vertexOffset);

  vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer.Get(), 0, VK_INDEX_TYPE_UINT16);

  std::size_t firstCommand = 0;

  while (firstCommand < m_quadCommands.size())
  {
    const VulkanTexture2D* texture = m_quadCommands[firstCommand].texture;

    std::size_t batchCommandCount = 0;

    while (firstCommand + batchCommandCount < m_quadCommands.size() &&
           m_quadCommands[firstCommand + batchCommandCount].texture == texture)
    {
      ++batchCommandCount;
    }

    RenderBatch(commandBuffer, frameIndex, firstCommand, batchCommandCount);

    firstCommand += batchCommandCount;
  }

  m_quadCommands.clear();
}

VulkanShaderModuleDesc VulkanRenderer2D::MakeShaderModuleDesc(
    const std::filesystem::path& path) const
{
  VulkanShaderModuleDesc desc{};
  desc.device = m_device;
  desc.path = path;

  return desc;
}

VulkanGraphicsPipelineDesc VulkanRenderer2D::MakePipelineDesc(VkFormat colorFormat) const
{
  VulkanGraphicsPipelineDesc desc{};
  desc.device = m_device;
  desc.vertexShader = m_vertexShader.Get();
  desc.fragmentShader = m_fragmentShader.Get();
  desc.colorFormat = colorFormat;

  desc.descriptorSetLayouts = {m_textureDescriptorSetLayout.Get()};

  desc.vertexBindings = {Renderer2DVertexBindingDescription()};

  const auto attributes = Renderer2DVertexAttributeDescriptions();

  desc.vertexAttributes = {attributes[0], attributes[1], attributes[2]};

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(ProjectionPushConstants);

  desc.pushConstantRanges = {pushConstantRange};

  return desc;
}

VulkanBufferDesc VulkanRenderer2D::MakeFrameVertexBufferDesc() const
{
  VulkanBufferDesc desc{};
  desc.physicalDevice = m_physicalDevice;
  desc.device = m_device;
  desc.size = sizeof(Renderer2DVertex) * static_cast<VkDeviceSize>(m_maxQuadsPerBatch) * 4;

  desc.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  desc.memoryProperties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  return desc;
}

VulkanBufferDesc VulkanRenderer2D::MakeIndexBufferDesc() const
{
  VulkanBufferDesc desc{};
  desc.physicalDevice = m_physicalDevice;
  desc.device = m_device;
  desc.size = sizeof(std::uint16_t) * static_cast<VkDeviceSize>(m_maxQuadsPerBatch) * 6;

  desc.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  desc.memoryProperties =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  return desc;
}

VulkanDescriptorSetLayoutDesc VulkanRenderer2D::MakeTextureDescriptorSetLayoutDesc() const
{
  VkDescriptorSetLayoutBinding textureBinding{};
  textureBinding.binding = 0;
  textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureBinding.descriptorCount = 1;
  textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  textureBinding.pImmutableSamplers = nullptr;

  VulkanDescriptorSetLayoutDesc desc{};
  desc.device = m_device;
  desc.bindings = {textureBinding};

  return desc;
}

VulkanTexture2DDesc VulkanRenderer2D::MakeWhiteTextureDesc() const
{
  static const std::uint32_t whitePixel = 0xFFFFFFFFu;

  VulkanTexture2DDesc desc{};
  desc.physicalDevice = m_physicalDevice;
  desc.device = m_device;
  desc.commandPool = m_commandPool;
  desc.graphicsQueue = m_graphicsQueue;
  desc.width = 1;
  desc.height = 1;
  desc.format = VK_FORMAT_R8G8B8A8_UNORM;
  desc.pixels = &whitePixel;
  desc.pixelsSize = sizeof(whitePixel);

  return desc;
}

void VulkanRenderer2D::CreateDescriptorPool()
{
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = 1024;

  VkDescriptorPoolCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  createInfo.flags = 0;
  createInfo.maxSets = 1024;
  createInfo.poolSizeCount = 1;
  createInfo.pPoolSizes = &poolSize;

  VkResult result = vkCreateDescriptorPool(m_device, &createInfo, nullptr, &m_descriptorPool);

  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create VulkanRenderer2D descriptor pool.");
  }
}

void VulkanRenderer2D::DestroyDescriptorPool()
{
  if (m_descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    m_descriptorPool = VK_NULL_HANDLE;
  }
}

std::vector<VulkanBuffer> VulkanRenderer2D::CreateFrameVertexBuffers() const
{
  std::vector<VulkanBuffer> buffers;
  buffers.reserve(m_framesInFlight);

  const VulkanBufferDesc desc = MakeFrameVertexBufferDesc();

  for (std::uint32_t i = 0; i < m_framesInFlight; ++i)
  {
    buffers.emplace_back(desc);
  }

  return buffers;
}

void VulkanRenderer2D::UploadIndexBuffer()
{
  const std::vector<std::uint16_t> indices = BuildQuadIndices(m_maxQuadsPerBatch);

  m_indexBuffer.Upload(indices.data(), sizeof(std::uint16_t) * indices.size());
}

void VulkanRenderer2D::UploadQueuedVertices(std::uint32_t frameIndex)
{
  if (m_quadCommands.empty())
  {
    return;
  }

  if (frameIndex >= m_frameVertexBuffers.size())
  {
    throw std::runtime_error("Cannot upload 2D vertices: frame index is invalid.");
  }

  if (m_quadCommands.size() > m_maxQuadsPerBatch)
  {
    throw std::runtime_error("Cannot upload 2D vertices: too many quads.");
  }

  std::vector<Renderer2DVertex> vertices;
  vertices.resize(m_quadCommands.size() * 4);

  for (std::size_t i = 0; i < m_quadCommands.size(); ++i)
  {
    WriteQuadVertices(vertices.data(), i, m_quadCommands[i]);
  }

  VulkanBuffer& vertexBuffer = m_frameVertexBuffers[frameIndex];

  vertexBuffer.Upload(vertices.data(), sizeof(Renderer2DVertex) * vertices.size());
}

void VulkanRenderer2D::RecreatePipeline(VkFormat colorFormat)
{
  m_colorFormat = colorFormat;
  m_pipeline = VulkanGraphicsPipeline(MakePipelineDesc(m_colorFormat));
}

VkDescriptorSet VulkanRenderer2D::AllocateTextureDescriptorSet(const VulkanTexture2D& texture)
{
  if (m_descriptorPool == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot allocate texture descriptor set: descriptor pool is null.");
  }

  VkDescriptorSetLayout layout = m_textureDescriptorSetLayout.Get();

  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = m_descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &layout;

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  VkResult result = vkAllocateDescriptorSets(m_device, &allocateInfo, &descriptorSet);

  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate Vulkan texture descriptor set.");
  }

  VkDescriptorImageInfo imageInfo{};
  imageInfo.sampler = texture.Sampler();
  imageInfo.imageView = texture.ImageView();
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = descriptorSet;
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);

  return descriptorSet;
}

void VulkanRenderer2D::RenderBatch(
    VkCommandBuffer commandBuffer,
    std::uint32_t frameIndex,
    std::size_t firstCommand,
    std::size_t commandCount)
{
  (void)frameIndex;

  if (commandCount == 0)
  {
    return;
  }

  const VulkanTexture2D* texture = m_quadCommands[firstCommand].texture;

  if (texture == nullptr || texture->DescriptorSet() == VK_NULL_HANDLE)
  {
    texture = &m_whiteTexture;
  }

  VkDescriptorSet descriptorSet = texture->DescriptorSet();

  vkCmdBindDescriptorSets(
      commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      m_pipeline.Layout(),
      0,
      1,
      &descriptorSet,
      0,
      nullptr);

  vkCmdDrawIndexed(
      commandBuffer,
      static_cast<std::uint32_t>(commandCount * 6),
      1,
      static_cast<std::uint32_t>(firstCommand * 6),
      0,
      0);
}

}  // namespace eng::vk
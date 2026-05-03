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

std::array<VkVertexInputAttributeDescription, 2> Renderer2DVertexAttributeDescriptions()
{
  std::array<VkVertexInputAttributeDescription, 2> attributes{};

  attributes[0].location = 0;
  attributes[0].binding = 0;
  attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[0].offset = offsetof(Renderer2DVertex, position);

  attributes[1].location = 1;
  attributes[1].binding = 0;
  attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributes[1].offset = offsetof(Renderer2DVertex, color);

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

  const std::size_t base = quadIndex * 4;

  vertices[base + 0] = Renderer2DVertex{{x0, y0}, {r, g, b, a}};
  vertices[base + 1] = Renderer2DVertex{{x1, y0}, {r, g, b, a}};
  vertices[base + 2] = Renderer2DVertex{{x1, y1}, {r, g, b, a}};
  vertices[base + 3] = Renderer2DVertex{{x0, y1}, {r, g, b, a}};
}

}  // namespace

VulkanRenderer2D::VulkanRenderer2D(const VulkanRenderer2DDesc& desc)
    : m_physicalDevice(desc.physicalDevice),
      m_device(desc.device),
      m_colorFormat(desc.colorFormat),
      m_framesInFlight(desc.framesInFlight),
      m_maxQuadsPerBatch(desc.maxQuadsPerBatch),
      m_vertexShader(MakeShaderModuleDesc(desc.vertexShaderPath)),
      m_fragmentShader(MakeShaderModuleDesc(desc.fragmentShaderPath)),
      m_pipeline(MakePipelineDesc(desc.colorFormat)),
      m_frameVertexBuffers(CreateFrameVertexBuffers()),
      m_indexBuffer(MakeIndexBufferDesc())
{
  if (m_physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanRenderer2D: physical device is null.");
  }

  if (m_device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanRenderer2D: device is null.");
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

  m_quadCommands.reserve(m_maxQuadsPerBatch);
}

VulkanRenderer2D::~VulkanRenderer2D() = default;

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

  QuadDrawCommand command{};
  command.x = x;
  command.y = y;
  command.width = width;
  command.height = height;
  command.color = color;

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
    const std::size_t remaining = m_quadCommands.size() - firstCommand;

    const std::size_t batchCommandCount = std::min<std::size_t>(remaining, m_maxQuadsPerBatch);

    RenderBatch(commandBuffer, extent, frameIndex, firstCommand, batchCommandCount);

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

  desc.vertexBindings = {Renderer2DVertexBindingDescription()};

  const auto attributes = Renderer2DVertexAttributeDescriptions();

  desc.vertexAttributes = {attributes[0], attributes[1]};

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

void VulkanRenderer2D::RecreatePipeline(VkFormat colorFormat)
{
  m_colorFormat = colorFormat;
  m_pipeline = VulkanGraphicsPipeline(MakePipelineDesc(m_colorFormat));
}

void VulkanRenderer2D::RenderBatch(
    VkCommandBuffer commandBuffer,
    VkExtent2D extent,
    std::uint32_t frameIndex,
    std::size_t firstCommand,
    std::size_t commandCount)
{
  (void)extent;

  if (commandCount == 0)
  {
    return;
  }

  std::vector<Renderer2DVertex> vertices;
  vertices.resize(commandCount * 4);

  for (std::size_t i = 0; i < commandCount; ++i)
  {
    WriteQuadVertices(vertices.data(), i, m_quadCommands[firstCommand + i]);
  }

  VulkanBuffer& vertexBuffer = m_frameVertexBuffers[frameIndex];

  vertexBuffer.Upload(vertices.data(), sizeof(Renderer2DVertex) * vertices.size());

  vkCmdDrawIndexed(commandBuffer, static_cast<std::uint32_t>(commandCount * 6), 1, 0, 0, 0);
}

}  // namespace eng::vk
#include "vk/VulkanGraphicsPipeline.h"

#include <array>
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

VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanGraphicsPipelineDesc& desc)
{
  Create(desc);
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
  Destroy();
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept
{
  m_device = other.m_device;
  m_pipelineLayout = other.m_pipelineLayout;
  m_pipeline = other.m_pipeline;

  other.m_device = VK_NULL_HANDLE;
  other.m_pipelineLayout = VK_NULL_HANDLE;
  other.m_pipeline = VK_NULL_HANDLE;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::operator=(VulkanGraphicsPipeline&& other) noexcept
{
  if (this != &other)
  {
    Destroy();

    m_device = other.m_device;
    m_pipelineLayout = other.m_pipelineLayout;
    m_pipeline = other.m_pipeline;

    other.m_device = VK_NULL_HANDLE;
    other.m_pipelineLayout = VK_NULL_HANDLE;
    other.m_pipeline = VK_NULL_HANDLE;
  }

  return *this;
}

VkPipeline VulkanGraphicsPipeline::Get() const
{
  return m_pipeline;
}

VkPipelineLayout VulkanGraphicsPipeline::Layout() const
{
  return m_pipelineLayout;
}

void VulkanGraphicsPipeline::Create(const VulkanGraphicsPipelineDesc& desc)
{
  if (desc.device == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanGraphicsPipeline: device is null.");
  }

  if (desc.vertexShader == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanGraphicsPipeline: vertex shader is null.");
  }

  if (desc.fragmentShader == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Cannot create VulkanGraphicsPipeline: fragment shader is null.");
  }

  if (desc.colorFormat == VK_FORMAT_UNDEFINED)
  {
    throw std::runtime_error("Cannot create VulkanGraphicsPipeline: color format is undefined.");
  }

  m_device = desc.device;

  VkPipelineLayoutCreateInfo layoutCreateInfo{};
  layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layoutCreateInfo.setLayoutCount = 0;
  layoutCreateInfo.pSetLayouts = nullptr;

  layoutCreateInfo.pushConstantRangeCount =
      static_cast<std::uint32_t>(desc.pushConstantRanges.size());

  layoutCreateInfo.pPushConstantRanges =
      desc.pushConstantRanges.empty() ? nullptr : desc.pushConstantRanges.data();

  vkCheck(
      vkCreatePipelineLayout(m_device, &layoutCreateInfo, nullptr, &m_pipelineLayout),
      "Failed to create Vulkan pipeline layout.");

  VkPipelineShaderStageCreateInfo vertexStage{};
  vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexStage.module = desc.vertexShader;
  vertexStage.pName = "main";

  VkPipelineShaderStageCreateInfo fragmentStage{};
  fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentStage.module = desc.fragmentShader;
  fragmentStage.pName = "main";

  const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertexStage, fragmentStage};

  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  vertexInput.vertexBindingDescriptionCount =
      static_cast<std::uint32_t>(desc.vertexBindings.size());
  vertexInput.pVertexBindingDescriptions =
      desc.vertexBindings.empty() ? nullptr : desc.vertexBindings.data();

  vertexInput.vertexAttributeDescriptionCount =
      static_cast<std::uint32_t>(desc.vertexAttributes.size());
  vertexInput.pVertexAttributeDescriptions =
      desc.vertexAttributes.empty() ? nullptr : desc.vertexAttributes.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = 1.0f;
  viewport.height = 1.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = {1, 1};

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.sampleShadingEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  const std::array<VkDynamicState, 2> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineRenderingCreateInfo renderingCreateInfo{};
  renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  renderingCreateInfo.colorAttachmentCount = 1;
  renderingCreateInfo.pColorAttachmentFormats = &desc.colorFormat;
  renderingCreateInfo.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
  renderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.pNext = &renderingCreateInfo;

  pipelineCreateInfo.stageCount = static_cast<std::uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();

  pipelineCreateInfo.pVertexInputState = &vertexInput;
  pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pRasterizationState = &rasterizer;
  pipelineCreateInfo.pMultisampleState = &multisampling;
  pipelineCreateInfo.pDepthStencilState = nullptr;
  pipelineCreateInfo.pColorBlendState = &colorBlending;
  pipelineCreateInfo.pDynamicState = &dynamicState;

  pipelineCreateInfo.layout = m_pipelineLayout;
  pipelineCreateInfo.renderPass = VK_NULL_HANDLE;
  pipelineCreateInfo.subpass = 0;
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineCreateInfo.basePipelineIndex = -1;

  vkCheck(
      vkCreateGraphicsPipelines(
          m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline),
      "Failed to create Vulkan graphics pipeline.");
}

void VulkanGraphicsPipeline::Destroy()
{
  if (m_pipeline != VK_NULL_HANDLE)
  {
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    m_pipeline = VK_NULL_HANDLE;
  }

  if (m_pipelineLayout != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
  }

  m_device = VK_NULL_HANDLE;
}

}  // namespace eng::vk
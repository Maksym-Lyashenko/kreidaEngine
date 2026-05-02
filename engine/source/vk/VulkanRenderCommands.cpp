#include "vk/VulkanRenderCommands.h"

#include <stdexcept>

namespace eng::vk
{

namespace
{

void ValidateCommandBuffer(VkCommandBuffer commandBuffer)
{
  if (commandBuffer == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Vulkan command buffer is null.");
  }
}

void ValidateColorImage(VkImage image)
{
  if (image == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Vulkan color image is null.");
  }
}

void ValidateImageView(VkImageView imageView)
{
  if (imageView == VK_NULL_HANDLE)
  {
    throw std::runtime_error("Vulkan image view is null.");
  }
}

void ValidateExtent(VkExtent2D extent)
{
  if (extent.width == 0 || extent.height == 0)
  {
    throw std::runtime_error("Vulkan rendering extent is zero.");
  }
}

void FillColorImageTransitionMasks(
    VkImageLayout oldLayout, VkImageLayout newLayout, VkImageMemoryBarrier2& barrier)
{
  if (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
  {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    barrier.srcAccessMask = 0;

    barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    return;
  }

  if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
      newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
  {
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

    barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
    barrier.dstAccessMask = 0;

    return;
  }

  throw std::runtime_error("Unsupported Vulkan color image layout transition.");
}

}  // namespace

void CmdTransitionColorImage(
    VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
  ValidateCommandBuffer(commandBuffer);
  ValidateColorImage(image);

  if (oldLayout == newLayout)
  {
    return;
  }

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

  FillColorImageTransitionMasks(oldLayout, newLayout, barrier);

  VkDependencyInfo dependencyInfo{};
  dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  dependencyInfo.imageMemoryBarrierCount = 1;
  dependencyInfo.pImageMemoryBarriers = &barrier;

  vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void CmdBeginColorRendering(VkCommandBuffer commandBuffer, const VulkanColorRenderingDesc& desc)
{
  ValidateCommandBuffer(commandBuffer);
  ValidateImageView(desc.imageView);
  ValidateExtent(desc.extent);

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = desc.imageView;
  colorAttachment.imageLayout = desc.imageLayout;
  colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
  colorAttachment.resolveImageView = VK_NULL_HANDLE;
  colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.loadOp = desc.loadOp;
  colorAttachment.storeOp = desc.storeOp;
  colorAttachment.clearValue = desc.clearValue;

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = desc.extent;
  renderingInfo.layerCount = 1;
  renderingInfo.viewMask = 0;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;
  renderingInfo.pDepthAttachment = nullptr;
  renderingInfo.pStencilAttachment = nullptr;

  vkCmdBeginRendering(commandBuffer, &renderingInfo);
}

void CmdEndRendering(VkCommandBuffer commandBuffer)
{
  ValidateCommandBuffer(commandBuffer);

  vkCmdEndRendering(commandBuffer);
}

void CmdSetViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent)
{
  ValidateCommandBuffer(commandBuffer);
  ValidateExtent(extent);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

}  // namespace eng::vk
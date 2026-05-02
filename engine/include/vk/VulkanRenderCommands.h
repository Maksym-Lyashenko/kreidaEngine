#pragma once

#include <vulkan/vulkan.h>

namespace eng::vk
{

struct VulkanColorRenderingDesc final
{
  VkImageView imageView = VK_NULL_HANDLE;
  VkImageLayout imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  VkExtent2D extent{};

  VkClearValue clearValue{};

  VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
};

void CmdTransitionColorImage(
    VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

void CmdBeginColorRendering(VkCommandBuffer commandBuffer, const VulkanColorRenderingDesc& desc);

void CmdEndRendering(VkCommandBuffer commandBuffer);

void CmdSetViewportAndScissor(VkCommandBuffer commandBuffer, VkExtent2D extent);

}  // namespace eng::vk
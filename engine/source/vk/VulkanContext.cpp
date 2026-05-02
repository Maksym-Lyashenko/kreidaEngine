#include "vk/VulkanContext.h"

#include "vk/VulkanSurfaceSDL.h"
#include "vk/VulkanRenderCommands.h"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <vector>

namespace eng::vk
{

namespace
{

SDL_Window* RequireWindow(SDL_Window* window)
{
  if (window == nullptr)
  {
    throw std::runtime_error("Cannot create VulkanContext: SDL_Window is null.");
  }

  return window;
}

VulkanInstance CreateInstance()
{
  VulkanInstanceDesc desc{};
  desc.appName = "Kreida Engine";
  desc.engineName = "Kreida Engine";
  desc.targetApiVersion = VK_API_VERSION_1_3;

#ifndef NDEBUG
  desc.enableValidation = true;
  desc.enableDebugUtils = true;
#else
  desc.enableValidation = false;
  desc.enableDebugUtils = false;
#endif

  const std::vector<const char*> sdlExtensions = GetRequiredSDLVulkanInstanceExtensions();

  desc.requiredInstanceExtensions.insert(
      desc.requiredInstanceExtensions.end(), sdlExtensions.begin(), sdlExtensions.end());

  return VulkanInstance(desc);
}

VulkanPhysicalDeviceDesc MakePhysicalDeviceDesc(VkInstance instance, VkSurfaceKHR surface)
{
  VulkanPhysicalDeviceDesc desc{};
  desc.instance = instance;
  desc.surface = surface;
  desc.targetApiVersion = VK_API_VERSION_1_3;
  desc.preferDiscreteGpu = true;
  desc.requireDynamicRendering = true;
  desc.requireSynchronization2 = true;

  return desc;
}

VulkanDeviceDesc MakeDeviceDesc(const VulkanPhysicalDevice& physicalDevice)
{
  VulkanDeviceDesc desc{};
  desc.physicalDevice = physicalDevice.Get();
  desc.queueFamilies = physicalDevice.QueueFamilies();
  desc.enableDynamicRendering = true;
  desc.enableSynchronization2 = true;

  return desc;
}

bool IsWindowRenderable(SDL_Window* window)
{
  if (window == nullptr)
  {
    return false;
  }

  const SDL_WindowFlags flags = SDL_GetWindowFlags(window);

  if ((flags & SDL_WINDOW_MINIMIZED) != 0)
  {
    return false;
  }

  int width = 0;
  int height = 0;

  if (!SDL_GetWindowSizeInPixels(window, &width, &height))
  {
    return false;
  }

  return width > 0 && height > 0;
}

void vkCheck(VkResult result, const char* message)
{
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error(message);
  }
}

bool IsSwapchainOutOfDate(VkResult result)
{
  return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR;
}

}  // namespace

VulkanContext::VulkanContext(SDL_Window* window)
    : m_window(RequireWindow(window)),
      m_instance(CreateInstance()),
      m_surface(CreateSDLVulkanSurface(m_instance.Get(), m_window)),
      m_physicalDevice(MakePhysicalDeviceDesc(m_instance.Get(), m_surface.Get())),
      m_device(MakeDeviceDesc(m_physicalDevice)),
      m_swapchain(MakeSwapchainDesc(m_window)),
      m_graphicsCommandPool(MakeGraphicsCommandPoolDesc()),
      m_frameCommandBuffers(CreateFrameCommandBuffers()),
      m_frameSyncObjects(CreateFrameSyncObjects()),
      m_renderFinishedSemaphores(CreateRenderFinishedSemaphores()),
      m_triangleVertexShader(MakeShaderModuleDesc("assets/shaders/triangle.vert.spv")),
      m_triangleFragmentShader(MakeShaderModuleDesc("assets/shaders/triangle.frag.spv")),
      m_trianglePipeline(MakeTrianglePipelineDesc())
{
  ResetSwapchainImageLayouts();

  SDL_Log("Selected Vulkan physical device: %s", m_physicalDevice.Properties().deviceName);

  SDL_Log(
      "Vulkan swapchain created: %ux%u, images: %u",
      m_swapchain.Extent().width,
      m_swapchain.Extent().height,
      m_swapchain.ImageCount());

  SDL_Log("Vulkan frame resources created. Frames in flight: %u", MaxFramesInFlight);

  SDL_Log("Vulkan triangle pipeline created.");
}

VulkanContext::~VulkanContext()
{
  m_device.WaitIdle();
}

void VulkanContext::NotifyWindowResized()
{
  m_windowResized = true;
}

void VulkanContext::DrawFrame()
{
  if (!IsWindowRenderable(m_window))
  {
    m_windowResized = true;
    return;
  }

  if (m_windowResized)
  {
    if (!RecreateSwapchain())
    {
      return;
    }
  }

  VulkanFrameSync& frameSync = m_frameSyncObjects[m_currentFrame];
  VulkanCommandBuffer& commandBuffer = m_frameCommandBuffers[m_currentFrame];

  frameSync.WaitFence();

  std::uint32_t imageIndex = 0;

  VkResult acquireResult = vkAcquireNextImageKHR(
      m_device.Get(),
      m_swapchain.Get(),
      UINT64_MAX,
      frameSync.ImageAvailableSemaphore(),
      VK_NULL_HANDLE,
      &imageIndex);

  if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
  {
    if (!RecreateSwapchain())
    {
      return;
    }

    return;
  }

  if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("Failed to acquire Vulkan swapchain image.");
  }

  const bool shouldRecreateSwapchain = acquireResult == VK_SUBOPTIMAL_KHR;

  frameSync.ResetFence();

  commandBuffer.Reset();
  commandBuffer.BeginOneTimeSubmit();

  RecordClearCommandBuffer(commandBuffer.Get(), imageIndex);

  commandBuffer.End();

  VkSemaphoreSubmitInfo waitSemaphoreInfo{};
  waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  waitSemaphoreInfo.semaphore = frameSync.ImageAvailableSemaphore();
  waitSemaphoreInfo.value = 0;
  waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  waitSemaphoreInfo.deviceIndex = 0;

  VkCommandBufferSubmitInfo commandBufferInfo{};
  commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  commandBufferInfo.commandBuffer = commandBuffer.Get();
  commandBufferInfo.deviceMask = 0;

  VkSemaphore renderFinishedSemaphore = m_renderFinishedSemaphores[imageIndex].Get();

  VkSemaphoreSubmitInfo signalSemaphoreInfo{};
  signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signalSemaphoreInfo.semaphore = renderFinishedSemaphore;
  signalSemaphoreInfo.value = 0;
  signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  signalSemaphoreInfo.deviceIndex = 0;

  VkSubmitInfo2 submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submitInfo.waitSemaphoreInfoCount = 1;
  submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
  submitInfo.commandBufferInfoCount = 1;
  submitInfo.pCommandBufferInfos = &commandBufferInfo;
  submitInfo.signalSemaphoreInfoCount = 1;
  submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

  vkCheck(
      vkQueueSubmit2(m_device.GraphicsQueue(), 1, &submitInfo, frameSync.InFlightFence()),
      "Failed to submit Vulkan command buffer.");

  VkSwapchainKHR swapchain = m_swapchain.Get();

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;

  presentInfo.pWaitSemaphores = &renderFinishedSemaphore;

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr;

  VkResult presentResult = vkQueuePresentKHR(m_device.PresentQueue(), &presentInfo);

  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR ||
      shouldRecreateSwapchain || m_windowResized)
  {
    if (!RecreateSwapchain())
    {
      return;
    }
  }
  else if (presentResult != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to present Vulkan swapchain image.");
  }

  m_currentFrame = (m_currentFrame + 1) % MaxFramesInFlight;
}

VkInstance VulkanContext::Instance() const
{
  return m_instance.Get();
}

VkSurfaceKHR VulkanContext::Surface() const
{
  return m_surface.Get();
}

VkPhysicalDevice VulkanContext::PhysicalDevice() const
{
  return m_physicalDevice.Get();
}

VkDevice VulkanContext::Device() const
{
  return m_device.Get();
}

VkQueue VulkanContext::GraphicsQueue() const
{
  return m_device.GraphicsQueue();
}

VkQueue VulkanContext::PresentQueue() const
{
  return m_device.PresentQueue();
}

VkCommandPool VulkanContext::GraphicsCommandPool() const
{
  return m_graphicsCommandPool.Get();
}

const VulkanPhysicalDevice& VulkanContext::PhysicalDeviceInfo() const
{
  return m_physicalDevice;
}

const VulkanDevice& VulkanContext::DeviceInfo() const
{
  return m_device;
}

const VulkanSwapchain& VulkanContext::Swapchain() const
{
  return m_swapchain;
}

VulkanSwapchainDesc VulkanContext::MakeSwapchainDesc(SDL_Window* window) const
{
  VulkanSwapchainDesc desc{};
  desc.physicalDevice = m_physicalDevice.Get();
  desc.device = m_device.Get();
  desc.surface = m_surface.Get();
  desc.window = window;
  desc.queueFamilies = m_physicalDevice.QueueFamilies();

  desc.vsync = true;
  desc.preferMailboxPresentMode = false;

  desc.preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
  desc.preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

  return desc;
}

VulkanCommandPoolDesc VulkanContext::MakeGraphicsCommandPoolDesc() const
{
  VulkanCommandPoolDesc desc{};
  desc.device = m_device.Get();
  desc.queueFamilyIndex = m_device.GraphicsQueueFamily();
  desc.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  return desc;
}

VulkanCommandBufferDesc VulkanContext::MakeFrameCommandBufferDesc() const
{
  VulkanCommandBufferDesc desc{};
  desc.device = m_device.Get();
  desc.commandPool = m_graphicsCommandPool.Get();
  desc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  return desc;
}

VulkanFrameSyncDesc VulkanContext::MakeFrameSyncDesc() const
{
  VulkanFrameSyncDesc desc{};
  desc.device = m_device.Get();
  desc.createFenceSignaled = true;

  return desc;
}

VulkanShaderModuleDesc VulkanContext::MakeShaderModuleDesc(const char* path) const
{
  VulkanShaderModuleDesc desc{};
  desc.device = m_device.Get();
  desc.path = path;

  return desc;
}

VulkanGraphicsPipelineDesc VulkanContext::MakeTrianglePipelineDesc() const
{
  VulkanGraphicsPipelineDesc desc{};
  desc.device = m_device.Get();
  desc.vertexShader = m_triangleVertexShader.Get();
  desc.fragmentShader = m_triangleFragmentShader.Get();
  desc.colorFormat = m_swapchain.ImageFormat();

  return desc;
}

std::vector<VulkanCommandBuffer> VulkanContext::CreateFrameCommandBuffers() const
{
  std::vector<VulkanCommandBuffer> commandBuffers;
  commandBuffers.reserve(MaxFramesInFlight);

  const VulkanCommandBufferDesc desc = MakeFrameCommandBufferDesc();

  for (std::uint32_t i = 0; i < MaxFramesInFlight; ++i)
  {
    commandBuffers.emplace_back(desc);
  }

  return commandBuffers;
}

std::vector<VulkanFrameSync> VulkanContext::CreateFrameSyncObjects() const
{
  std::vector<VulkanFrameSync> syncObjects;
  syncObjects.reserve(MaxFramesInFlight);

  const VulkanFrameSyncDesc desc = MakeFrameSyncDesc();

  for (std::uint32_t i = 0; i < MaxFramesInFlight; ++i)
  {
    syncObjects.emplace_back(desc);
  }

  return syncObjects;
}

std::vector<VulkanSemaphore> VulkanContext::CreateRenderFinishedSemaphores() const
{
  std::vector<VulkanSemaphore> semaphores;
  semaphores.reserve(m_swapchain.ImageCount());

  VulkanSemaphoreDesc desc{};
  desc.device = m_device.Get();

  for (std::uint32_t i = 0; i < m_swapchain.ImageCount(); ++i)
  {
    semaphores.emplace_back(desc);
  }

  return semaphores;
}

bool VulkanContext::RecreateSwapchain()
{
  if (!IsWindowRenderable(m_window))
  {
    m_windowResized = true;
    return false;
  }

  m_device.WaitIdle();

  m_renderFinishedSemaphores.clear();

  m_swapchain.Recreate(MakeSwapchainDesc(m_window));

  m_renderFinishedSemaphores = CreateRenderFinishedSemaphores();

  ResetSwapchainImageLayouts();

  m_windowResized = false;

  SDL_Log(
      "Vulkan swapchain recreated: %ux%u, images: %u",
      m_swapchain.Extent().width,
      m_swapchain.Extent().height,
      m_swapchain.ImageCount());

  return true;
}

void VulkanContext::RecordClearCommandBuffer(
    VkCommandBuffer commandBuffer, std::uint32_t imageIndex)
{
  const VkImage swapchainImage = m_swapchain.Images()[imageIndex];
  const VkImageView swapchainImageView = m_swapchain.ImageViews()[imageIndex];

  const VkImageLayout oldLayout = m_swapchainImageLayouts[imageIndex];

  CmdTransitionColorImage(
      commandBuffer, swapchainImage, oldLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  VkClearValue clearValue{};
  clearValue.color.float32[0] = 0.04f;
  clearValue.color.float32[1] = 0.05f;
  clearValue.color.float32[2] = 0.08f;
  clearValue.color.float32[3] = 1.0f;

  VulkanColorRenderingDesc renderingDesc{};
  renderingDesc.imageView = swapchainImageView;
  renderingDesc.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  renderingDesc.extent = m_swapchain.Extent();
  renderingDesc.clearValue = clearValue;
  renderingDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  renderingDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  CmdBeginColorRendering(commandBuffer, renderingDesc);

  CmdSetViewportAndScissor(commandBuffer, m_swapchain.Extent());

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_trianglePipeline.Get());

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  CmdEndRendering(commandBuffer);

  CmdTransitionColorImage(
      commandBuffer,
      swapchainImage,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  m_swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

void VulkanContext::ResetSwapchainImageLayouts()
{
  m_swapchainImageLayouts.clear();
  m_swapchainImageLayouts.resize(m_swapchain.ImageCount(), VK_IMAGE_LAYOUT_UNDEFINED);
}

}  // namespace eng::vk
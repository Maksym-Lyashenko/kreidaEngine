#pragma once

#include "vk/VulkanInstance.h"
#include "vk/VulkanSurface.h"
#include "vk/VulkanPhysicalDevice.h"
#include "vk/VulkanDevice.h"
#include "vk/VulkanSwapchain.h"
#include "vk/VulkanCommandPool.h"
#include "vk/VulkanCommandBuffer.h"
#include "vk/VulkanSemaphore.h"
#include "vk/VulkanFrameSync.h"
#include "vk/VulkanShaderModule.h"
#include "vk/VulkanGraphicsPipeline.h"
#include "vk/Vertex2D.h"
#include "vk/VulkanBuffer.h"
#include "vk/VulkanRenderer2D.h"

#include <cstdint>
#include <vector>

struct SDL_Window;

namespace eng::vk
{

class VulkanContext final
{
 public:
  explicit VulkanContext(SDL_Window* window);
  ~VulkanContext();

  VulkanContext(const VulkanContext&) = delete;
  VulkanContext& operator=(const VulkanContext&) = delete;

  VulkanContext(VulkanContext&&) = delete;
  VulkanContext& operator=(VulkanContext&&) = delete;

  void NotifyWindowResized();
  void DrawFrame();

  [[nodiscard]] VulkanRenderer2D& Renderer2D();
  [[nodiscard]] const VulkanRenderer2D& Renderer2D() const;

  [[nodiscard]] VkInstance Instance() const;
  [[nodiscard]] VkSurfaceKHR Surface() const;
  [[nodiscard]] VkPhysicalDevice PhysicalDevice() const;
  [[nodiscard]] VkDevice Device() const;

  [[nodiscard]] VkQueue GraphicsQueue() const;
  [[nodiscard]] VkQueue PresentQueue() const;

  [[nodiscard]] VkCommandPool GraphicsCommandPool() const;

  [[nodiscard]] const VulkanPhysicalDevice& PhysicalDeviceInfo() const;
  [[nodiscard]] const VulkanDevice& DeviceInfo() const;
  [[nodiscard]] const VulkanSwapchain& Swapchain() const;

 private:
  VulkanSwapchainDesc MakeSwapchainDesc(SDL_Window* window) const;
  VulkanCommandPoolDesc MakeGraphicsCommandPoolDesc() const;
  VulkanCommandBufferDesc MakeFrameCommandBufferDesc() const;
  VulkanFrameSyncDesc MakeFrameSyncDesc() const;
  VulkanShaderModuleDesc MakeShaderModuleDesc(const char* path) const;

  VulkanRenderer2DDesc MakeRenderer2DDesc() const;

  std::vector<VulkanCommandBuffer> CreateFrameCommandBuffers() const;
  std::vector<VulkanFrameSync> CreateFrameSyncObjects() const;
  std::vector<VulkanSemaphore> CreateRenderFinishedSemaphores() const;

  [[nodiscard]] bool RecreateSwapchain();
  void RecordFrameCommandBuffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex);
  void ResetSwapchainImageLayouts();

 private:
  static constexpr std::uint32_t MaxFramesInFlight = 2;

 private:
  SDL_Window* m_window = nullptr;

  VulkanInstance m_instance;
  VulkanSurface m_surface;
  VulkanPhysicalDevice m_physicalDevice;
  VulkanDevice m_device;
  VulkanSwapchain m_swapchain;

  VulkanCommandPool m_graphicsCommandPool;

  std::vector<VulkanCommandBuffer> m_frameCommandBuffers;
  std::vector<VulkanFrameSync> m_frameSyncObjects;

  std::vector<VulkanSemaphore> m_renderFinishedSemaphores;

  std::vector<VkImageLayout> m_swapchainImageLayouts;

  std::uint32_t m_currentFrame = 0;
  bool m_windowResized = false;

  VulkanRenderer2D m_renderer2D;
};

}  // namespace eng::vk
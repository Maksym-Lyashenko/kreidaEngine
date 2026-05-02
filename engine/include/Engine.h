#pragma once

#include <chrono>
#include <memory>

struct SDL_Window;

namespace eng
{

class Application;

namespace vk
{
class VulkanContext;
}

class Engine final
{
 public:
  static Engine& GetInstance();

  bool Init(int width, int height);
  void Run();
  void Destroy();

  void SetApplication(Application* app);
  Application* GetApplication();

  [[nodiscard]] SDL_Window* GetWindow() const;
  [[nodiscard]] vk::VulkanContext* GetVulkanContext() const;

 private:
  Engine() = default;
  ~Engine() = default;

 private:
  std::unique_ptr<Application> m_application;
  SDL_Window* m_window = nullptr;

  std::unique_ptr<vk::VulkanContext> m_vulkanContext;

  std::chrono::steady_clock::time_point m_lastTimePoint{};
};

}  // namespace eng
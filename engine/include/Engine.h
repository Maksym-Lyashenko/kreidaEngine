#pragma once

#include <chrono>
#include <filesystem>
#include <memory>

struct SDL_Window;

namespace eng
{

class Application;
class Renderer2D;
class Texture2D;
class AssetManager;

namespace vk
{

class VulkanContext;

}  // namespace vk

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
  [[nodiscard]] Renderer2D* GetRenderer2D();

  [[nodiscard]] AssetManager* GetAssetManager();
  [[nodiscard]] AssetManager& Assets();

 private:
  Engine() = default;
  ~Engine() = default;

 private:
  std::unique_ptr<Application> m_application;
  SDL_Window* m_window = nullptr;

  std::unique_ptr<vk::VulkanContext> m_vulkanContext;
  std::unique_ptr<Renderer2D> m_renderer2D;
  std::unique_ptr<AssetManager> m_assetManager;

  std::chrono::steady_clock::time_point m_lastTimePoint{};
};

}  // namespace eng
#include "Engine.h"

#include <SDL3/SDL.h>

#include <exception>

#include "Application.h"
#include "vk/VulkanContext.h"

namespace eng
{

Engine& Engine::GetInstance()
{
  static Engine instance;
  return instance;
}

bool Engine::Init(int width, int height)
{
  if (!m_application)
  {
    return false;
  }

  if (!SDL_Init(SDL_INIT_VIDEO))
  {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return false;
  }

  m_window =
      SDL_CreateWindow("Kreida Engine", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

  if (!m_window)
  {
    SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return false;
  }

  try
  {
    m_vulkanContext = std::make_unique<vk::VulkanContext>(m_window);
  }
  catch (const std::exception& e)
  {
    SDL_Log("VulkanContext creation failed: %s", e.what());

    SDL_DestroyWindow(m_window);
    m_window = nullptr;

    SDL_Quit();

    return false;
  }

  return m_application->Init();
}

void Engine::Run()
{
  if (!m_application)
  {
    return;
  }

  m_lastTimePoint = std::chrono::steady_clock::now();

  bool running = true;
  bool resized = false;

  while (running && !m_application->NeedsToBeClosed())
  {
    SDL_Event e;

    while (SDL_PollEvent(&e))
    {
      if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
      {
        running = false;
      }

      if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || e.type == SDL_EVENT_WINDOW_RESIZED ||
          e.type == SDL_EVENT_WINDOW_MINIMIZED || e.type == SDL_EVENT_WINDOW_RESTORED)
      {
        resized = true;
      }
    }

    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - m_lastTimePoint).count();
    m_lastTimePoint = now;

    m_application->Update(deltaTime);

    if (resized)
    {
      if (m_vulkanContext)
      {
        m_vulkanContext->NotifyWindowResized();
      }

      resized = false;
    }

    if (m_vulkanContext)
    {
      try
      {
        m_vulkanContext->DrawFrame();
      }
      catch (const std::exception& e)
      {
        SDL_Log("Vulkan DrawFrame failed: %s", e.what());
        running = false;
      }
    }
  }
}

void Engine::Destroy()
{
  if (m_application)
  {
    m_application->Destroy();
    m_application.reset();
  }

  m_vulkanContext.reset();

  if (m_window)
  {
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }

  SDL_Quit();
}

void Engine::SetApplication(Application* app)
{
  m_application.reset(app);
}

Application* Engine::GetApplication()
{
  return m_application.get();
}

SDL_Window* Engine::GetWindow() const
{
  return m_window;
}

vk::VulkanContext* Engine::GetVulkanContext() const
{
  return m_vulkanContext.get();
}

}  // namespace eng
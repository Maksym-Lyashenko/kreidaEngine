#pragma once

namespace eng
{

namespace vk
{

class VulkanRenderer2D;

}

struct Color4 final
{
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

class Renderer2D final
{
 public:
  Renderer2D() = default;
  explicit Renderer2D(vk::VulkanRenderer2D* backend);

  Renderer2D(const Renderer2D&) = delete;
  Renderer2D& operator=(const Renderer2D&) = delete;

  Renderer2D(Renderer2D&&) = delete;
  Renderer2D& operator=(Renderer2D&&) = delete;

  void SetBackend(vk::VulkanRenderer2D* backend);

  [[nodiscard]] bool IsValid() const;

  void BeginFrame();

  void DrawQuad(float x, float y, float width, float height, Color4 color);

 private:
  vk::VulkanRenderer2D* m_backend = nullptr;
};

}  // namespace eng
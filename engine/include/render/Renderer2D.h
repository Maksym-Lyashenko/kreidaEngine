#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace eng
{

namespace vk
{

class VulkanRenderer2D;

}

class Texture2D;
struct Sprite;

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

  [[nodiscard]] std::unique_ptr<Texture2D> CreateTexture2DFromPixels(
      std::uint32_t width, std::uint32_t height, const void* pixels, std::size_t pixelsSize);

  void DrawQuad(float x, float y, float width, float height, Color4 color);

  void DrawTexture(
      Texture2D* texture, float x, float y, float width, float height, Color4 tint = Color4{});

  void DrawTextureRegion(
      Texture2D* texture,
      float x,
      float y,
      float width,
      float height,
      float u0,
      float v0,
      float u1,
      float v1,
      Color4 tint = Color4{});

  void DrawTextureRegionPixels(
      Texture2D* texture,
      float x,
      float y,
      float width,
      float height,
      float sourceX,
      float sourceY,
      float sourceWidth,
      float sourceHeight,
      Color4 tint = Color4{});

  void DrawSprite(const Sprite& sprite, float x, float y, Color4 tint = Color4{});

  void DrawSprite(
      const Sprite& sprite, float x, float y, float width, float height, Color4 tint = Color4{});

  void DrawSpritePivoted(
      const Sprite& sprite, float x, float y, float scale = 1.0f, Color4 tint = Color4{});

  void DrawSpritePivoted(
      const Sprite& sprite, float x, float y, float width, float height, Color4 tint = Color4{});

 private:
  vk::VulkanRenderer2D* m_backend = nullptr;
};

}  // namespace eng
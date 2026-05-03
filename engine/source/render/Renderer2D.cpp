#include "render/Renderer2D.h"

#include "graphics/Texture2D.h"
#include "vk/VulkanRenderer2D.h"
#include "vk/VulkanTexture2D.h"

namespace eng
{

Renderer2D::Renderer2D(vk::VulkanRenderer2D* backend) : m_backend(backend)
{
}

void Renderer2D::SetBackend(vk::VulkanRenderer2D* backend)
{
  m_backend = backend;
}

bool Renderer2D::IsValid() const
{
  return m_backend != nullptr;
}

void Renderer2D::BeginFrame()
{
  if (m_backend == nullptr)
  {
    return;
  }

  m_backend->ClearQueuedCommands();
}

std::unique_ptr<Texture2D> Renderer2D::CreateTexture2DFromPixels(
    std::uint32_t width, std::uint32_t height, const void* pixels, std::size_t pixelsSize)
{
  if (m_backend == nullptr)
  {
    return nullptr;
  }

  if (width == 0 || height == 0 || pixels == nullptr || pixelsSize == 0)
  {
    return nullptr;
  }

  std::unique_ptr<vk::VulkanTexture2D> backendTexture = m_backend->CreateTexture2DFromPixels(
      width, height, pixels, static_cast<VkDeviceSize>(pixelsSize));

  if (!backendTexture)
  {
    return nullptr;
  }

  return std::make_unique<Texture2D>(std::move(backendTexture));
}

void Renderer2D::DrawQuad(float x, float y, float width, float height, Color4 color)
{
  if (m_backend == nullptr)
  {
    return;
  }

  m_backend->DrawQuad(x, y, width, height, vk::Color4{color.r, color.g, color.b, color.a});
}

void Renderer2D::DrawTexture(
    Texture2D* texture, float x, float y, float width, float height, Color4 tint)
{
  if (m_backend == nullptr || texture == nullptr || !texture->IsValid())
  {
    return;
  }

  vk::VulkanTexture2D* backendTexture = texture->Backend();

  if (backendTexture == nullptr)
  {
    return;
  }

  m_backend->DrawTexture(
      *backendTexture, x, y, width, height, vk::Color4{tint.r, tint.g, tint.b, tint.a});
}

void Renderer2D::DrawTextureRegion(
    Texture2D* texture,
    float x,
    float y,
    float width,
    float height,
    float u0,
    float v0,
    float u1,
    float v1,
    Color4 tint)
{
  if (m_backend == nullptr || texture == nullptr || !texture->IsValid())
  {
    return;
  }

  vk::VulkanTexture2D* backendTexture = texture->Backend();

  if (backendTexture == nullptr)
  {
    return;
  }

  m_backend->DrawTextureRegion(
      *backendTexture,
      x,
      y,
      width,
      height,
      u0,
      v0,
      u1,
      v1,
      vk::Color4{tint.r, tint.g, tint.b, tint.a});
}

void Renderer2D::DrawTextureRegionPixels(
    Texture2D* texture,
    float x,
    float y,
    float width,
    float height,
    float sourceX,
    float sourceY,
    float sourceWidth,
    float sourceHeight,
    Color4 tint)
{
  if (texture == nullptr || !texture->IsValid())
  {
    return;
  }

  const float textureWidth = static_cast<float>(texture->Width());

  const float textureHeight = static_cast<float>(texture->Height());

  if (textureWidth <= 0.0f || textureHeight <= 0.0f)
  {
    return;
  }

  const float u0 = sourceX / textureWidth;
  const float v0 = sourceY / textureHeight;
  const float u1 = (sourceX + sourceWidth) / textureWidth;
  const float v1 = (sourceY + sourceHeight) / textureHeight;

  DrawTextureRegion(texture, x, y, width, height, u0, v0, u1, v1, tint);
}

}  // namespace eng
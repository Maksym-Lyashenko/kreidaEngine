#include "render/Renderer2D.h"

#include "vk/VulkanRenderer2D.h"

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

void Renderer2D::DrawQuad(float x, float y, float width, float height, Color4 color)
{
  if (m_backend == nullptr)
  {
    return;
  }

  m_backend->DrawQuad(x, y, width, height, vk::Color4{color.r, color.g, color.b, color.a});
}

}  // namespace eng
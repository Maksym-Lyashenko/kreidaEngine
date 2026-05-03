#include "graphics/Texture2D.h"

#include "vk/VulkanTexture2D.h"

namespace eng
{

Texture2D::Texture2D(std::unique_ptr<vk::VulkanTexture2D> backend) : m_backend(std::move(backend))
{
}

Texture2D::~Texture2D() = default;

Texture2D::Texture2D(Texture2D&& other) noexcept = default;
Texture2D& Texture2D::operator=(Texture2D&& other) noexcept = default;

std::uint32_t Texture2D::Width() const
{
  return m_backend ? m_backend->Width() : 0;
}

std::uint32_t Texture2D::Height() const
{
  return m_backend ? m_backend->Height() : 0;
}

bool Texture2D::IsValid() const
{
  return m_backend != nullptr;
}

vk::VulkanTexture2D* Texture2D::Backend()
{
  return m_backend.get();
}

const vk::VulkanTexture2D* Texture2D::Backend() const
{
  return m_backend.get();
}

}  // namespace eng
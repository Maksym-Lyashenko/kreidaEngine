#pragma once

#include <cstdint>
#include <memory>

namespace eng
{

namespace vk
{

class VulkanTexture2D;

}

class Renderer2D;
class Engine;

class Texture2D final
{
 public:
  explicit Texture2D(std::unique_ptr<vk::VulkanTexture2D> backend);
  ~Texture2D();

  Texture2D(const Texture2D&) = delete;
  Texture2D& operator=(const Texture2D&) = delete;

  Texture2D(Texture2D&& other) noexcept;
  Texture2D& operator=(Texture2D&& other) noexcept;

  [[nodiscard]] std::uint32_t Width() const;
  [[nodiscard]] std::uint32_t Height() const;
  [[nodiscard]] bool IsValid() const;

 private:
  friend class Renderer2D;
  friend class Engine;

  [[nodiscard]] vk::VulkanTexture2D* Backend();
  [[nodiscard]] const vk::VulkanTexture2D* Backend() const;

 private:
  std::unique_ptr<vk::VulkanTexture2D> m_backend;
};

}  // namespace eng
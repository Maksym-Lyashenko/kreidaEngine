#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace eng
{

class Texture2D;

struct Sprite final
{
  std::shared_ptr<Texture2D> texture;

  float sourceX = 0.0f;
  float sourceY = 0.0f;
  float sourceWidth = 0.0f;
  float sourceHeight = 0.0f;

  // In source-frame pixels.
  float pivotX = 0.0f;
  float pivotY = 0.0f;

  [[nodiscard]] bool IsValid() const;
};

class SpriteSheet final
{
 public:
  SpriteSheet() = default;

  SpriteSheet(
      std::shared_ptr<Texture2D> texture,
      std::uint32_t spriteWidth,
      std::uint32_t spriteHeight,
      std::uint32_t margin = 0,
      std::uint32_t spacing = 0,
      float pivotX = 0.0f,
      float pivotY = 0.0f);

  [[nodiscard]] bool IsValid() const;

  [[nodiscard]] std::uint32_t SpriteWidth() const;
  [[nodiscard]] std::uint32_t SpriteHeight() const;

  [[nodiscard]] std::uint32_t Columns() const;
  [[nodiscard]] std::uint32_t Rows() const;
  [[nodiscard]] std::uint32_t Count() const;

  [[nodiscard]] Sprite GetSprite(std::uint32_t column, std::uint32_t row) const;

  [[nodiscard]] Sprite GetSpriteByIndex(std::uint32_t index) const;

  [[nodiscard]] std::vector<Sprite> GetSpritesInRow(
      std::uint32_t row, std::uint32_t firstColumn, std::uint32_t count) const;

  [[nodiscard]] std::vector<Sprite> GetSpritesByIndices(
      const std::vector<std::uint32_t>& indices) const;

 private:
  std::shared_ptr<Texture2D> m_texture;

  std::uint32_t m_spriteWidth = 0;
  std::uint32_t m_spriteHeight = 0;

  std::uint32_t m_margin = 0;
  std::uint32_t m_spacing = 0;

  float m_pivotX = 0.0f;
  float m_pivotY = 0.0f;
};

}  // namespace eng
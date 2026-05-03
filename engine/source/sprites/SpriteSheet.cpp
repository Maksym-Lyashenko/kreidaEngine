#include "sprites/SpriteSheet.h"

#include "graphics/Texture2D.h"

namespace eng
{

bool Sprite::IsValid() const
{
  return texture != nullptr && texture->IsValid() && sourceWidth > 0.0f && sourceHeight > 0.0f;
}

SpriteSheet::SpriteSheet(
    std::shared_ptr<Texture2D> texture,
    std::uint32_t spriteWidth,
    std::uint32_t spriteHeight,
    std::uint32_t margin,
    std::uint32_t spacing,
    float pivotX,
    float pivotY)
    : m_texture(std::move(texture)),
      m_spriteWidth(spriteWidth),
      m_spriteHeight(spriteHeight),
      m_margin(margin),
      m_spacing(spacing),
      m_pivotX(pivotX),
      m_pivotY(pivotY)
{
}

bool SpriteSheet::IsValid() const
{
  return m_texture != nullptr && m_texture->IsValid() && m_spriteWidth > 0 && m_spriteHeight > 0 &&
         Columns() > 0 && Rows() > 0;
}

std::uint32_t SpriteSheet::SpriteWidth() const
{
  return m_spriteWidth;
}

std::uint32_t SpriteSheet::SpriteHeight() const
{
  return m_spriteHeight;
}

std::uint32_t SpriteSheet::Columns() const
{
  if (m_texture == nullptr || !m_texture->IsValid())
  {
    return 0;
  }

  if (m_spriteWidth == 0)
  {
    return 0;
  }

  const std::uint32_t textureWidth = m_texture->Width();

  if (textureWidth < m_margin * 2 + m_spriteWidth)
  {
    return 0;
  }

  return (textureWidth - m_margin * 2 + m_spacing) / (m_spriteWidth + m_spacing);
}

std::uint32_t SpriteSheet::Rows() const
{
  if (m_texture == nullptr || !m_texture->IsValid())
  {
    return 0;
  }

  if (m_spriteHeight == 0)
  {
    return 0;
  }

  const std::uint32_t textureHeight = m_texture->Height();

  if (textureHeight < m_margin * 2 + m_spriteHeight)
  {
    return 0;
  }

  return (textureHeight - m_margin * 2 + m_spacing) / (m_spriteHeight + m_spacing);
}

std::uint32_t SpriteSheet::Count() const
{
  return Columns() * Rows();
}

Sprite SpriteSheet::GetSprite(std::uint32_t column, std::uint32_t row) const
{
  if (!IsValid())
  {
    return {};
  }

  if (column >= Columns() || row >= Rows())
  {
    return {};
  }

  const std::uint32_t sourceX = m_margin + column * (m_spriteWidth + m_spacing);

  const std::uint32_t sourceY = m_margin + row * (m_spriteHeight + m_spacing);

  Sprite sprite{};
  sprite.texture = m_texture;
  sprite.sourceX = static_cast<float>(sourceX);
  sprite.sourceY = static_cast<float>(sourceY);
  sprite.sourceWidth = static_cast<float>(m_spriteWidth);
  sprite.sourceHeight = static_cast<float>(m_spriteHeight);
  sprite.pivotX = m_pivotX;
  sprite.pivotY = m_pivotY;

  return sprite;
}

Sprite SpriteSheet::GetSpriteByIndex(std::uint32_t index) const
{
  const std::uint32_t columns = Columns();

  if (columns == 0)
  {
    return {};
  }

  const std::uint32_t column = index % columns;
  const std::uint32_t row = index / columns;

  return GetSprite(column, row);
}

std::vector<Sprite> SpriteSheet::GetSpritesInRow(
    std::uint32_t row, std::uint32_t firstColumn, std::uint32_t count) const
{
  std::vector<Sprite> sprites;
  sprites.reserve(count);

  for (std::uint32_t i = 0; i < count; ++i)
  {
    Sprite sprite = GetSprite(firstColumn + i, row);

    if (sprite.IsValid())
    {
      sprites.push_back(sprite);
    }
  }

  return sprites;
}

std::vector<Sprite> SpriteSheet::GetSpritesByIndices(
    const std::vector<std::uint32_t>& indices) const
{
  std::vector<Sprite> sprites;
  sprites.reserve(indices.size());

  for (std::uint32_t index : indices)
  {
    Sprite sprite = GetSpriteByIndex(index);

    if (sprite.IsValid())
    {
      sprites.push_back(sprite);
    }
  }

  return sprites;
}

}  // namespace eng
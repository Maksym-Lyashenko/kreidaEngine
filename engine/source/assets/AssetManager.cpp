#include "assets/AssetManager.h"

#include "graphics/Texture2D.h"
#include "render/Renderer2D.h"

#include <SDL3/SDL_log.h>

#include <stb_image.h>

namespace eng
{

AssetManager::AssetManager(Renderer2D* renderer) : m_renderer(renderer)
{
}

void AssetManager::SetRenderer(Renderer2D* renderer)
{
  m_renderer = renderer;
}

std::shared_ptr<Texture2D> AssetManager::LoadTexture2D(const std::filesystem::path& path)
{
  if (m_renderer == nullptr || !m_renderer->IsValid())
  {
    SDL_Log("LoadTexture2D failed: Renderer2D is not valid.");
    return nullptr;
  }

  const std::string cacheKey = NormalizePath(path);

  if (auto it = m_textureCache.find(cacheKey); it != m_textureCache.end())
  {
    if (std::shared_ptr<Texture2D> cachedTexture = it->second.lock())
    {
      return cachedTexture;
    }
  }

  int width = 0;
  int height = 0;
  int channels = 0;

  stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (pixels == nullptr)
  {
    SDL_Log("LoadTexture2D failed: %s", stbi_failure_reason());
    return nullptr;
  }

  if (width <= 0 || height <= 0)
  {
    SDL_Log("LoadTexture2D failed: invalid image size.");

    stbi_image_free(pixels);
    return nullptr;
  }

  const std::size_t pixelsSize =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;

  std::unique_ptr<Texture2D> texture = m_renderer->CreateTexture2DFromPixels(
      static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), pixels, pixelsSize);

  stbi_image_free(pixels);

  if (!texture || !texture->IsValid())
  {
    SDL_Log("LoadTexture2D failed: failed to create GPU texture.");
    return nullptr;
  }

  std::shared_ptr<Texture2D> sharedTexture(std::move(texture));
  m_textureCache[cacheKey] = sharedTexture;

  SDL_Log("Texture loaded: %s (%dx%d)", path.string().c_str(), width, height);

  return sharedTexture;
}

void AssetManager::Clear()
{
  m_textureCache.clear();
}

std::string AssetManager::NormalizePath(const std::filesystem::path& path)
{
  return path.lexically_normal().generic_string();
}

}  // namespace eng
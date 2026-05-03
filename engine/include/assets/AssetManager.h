#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace eng
{

class Renderer2D;
class Texture2D;

class AssetManager final
{
 public:
  AssetManager() = default;
  explicit AssetManager(Renderer2D* renderer);

  AssetManager(const AssetManager&) = delete;
  AssetManager& operator=(const AssetManager&) = delete;

  AssetManager(AssetManager&&) = delete;
  AssetManager& operator=(AssetManager&&) = delete;

  void SetRenderer(Renderer2D* renderer);

  [[nodiscard]] std::shared_ptr<Texture2D> LoadTexture2D(const std::filesystem::path& path);

  void Clear();

 private:
  [[nodiscard]] static std::string NormalizePath(const std::filesystem::path& path);

 private:
  Renderer2D* m_renderer = nullptr;

  std::unordered_map<std::string, std::weak_ptr<Texture2D>> m_textureCache;
};

}  // namespace eng
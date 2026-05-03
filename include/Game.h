#pragma once

#include <eng.h>

#include <memory>

class Game : public eng::Application
{
 public:
  bool Init() override;
  void Update(float DeltaTime) override;
  void Destroy() override;

 private:
  eng::Renderer2D* m_renderer = nullptr;
  std::shared_ptr<eng::Texture2D> m_texture;
};
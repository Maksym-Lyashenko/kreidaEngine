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
};
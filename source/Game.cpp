#include "Game.h"

#include <cmath>

bool Game::Init()
{
  m_renderer = eng::Engine::GetInstance().GetRenderer2D();

  return true;
}

void Game::Update(float DeltaTime)
{
  if (!m_renderer)
  {
    return;
  }

  static float time = 0.0f;
  time += DeltaTime;

  const float x = 100.0f + std::sin(time) * 60.0f;

  m_renderer->DrawQuad(x, 100.0f, 300.0f, 180.0f, eng::Color4{0.95f, 0.35f, 0.15f, 1.0f});

  m_renderer->DrawQuad(460.0f, 160.0f, 220.0f, 220.0f, eng::Color4{0.15f, 0.55f, 1.0f, 1.0f});
}

void Game::Destroy()
{
  m_renderer = nullptr;
}
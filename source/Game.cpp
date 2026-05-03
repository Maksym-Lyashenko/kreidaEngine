#include "Game.h"

#include <cmath>

bool Game::Init()
{
  eng::Engine& engine = eng::Engine::GetInstance();

  m_renderer = engine.GetRenderer2D();

  m_texture = engine.Assets().LoadTexture2D("assets/textures/brick.png");

  return m_renderer->IsValid();
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

  m_renderer->DrawQuad(x, x, 320.0f, 200.0f, eng::Color4{0.1f, 0.15f, 0.22f, 1.0f});

  if (m_texture)
  {
    // First 16x16 sprite from atlas.
    m_renderer->DrawTextureRegionPixels(
        m_texture.get(),
        460.0f,
        160.0f,
        128.0f,
        128.0f,
        0.0f,
        0.0f,
        16.0f,
        16.0f,
        eng::Color4{1.0f, 1.0f, 1.0f, 1.0f});

    // Second sprite: x = 16, y = 0.
    m_renderer->DrawTextureRegionPixels(
        m_texture.get(),
        620.0f,
        160.0f,
        128.0f,
        128.0f,
        16.0f,
        0.0f,
        16.0f,
        16.0f,
        eng::Color4{1.0f, 1.0f, 1.0f, 1.0f});
  }
}

void Game::Destroy()
{
  m_texture.reset();
  m_renderer = nullptr;
}
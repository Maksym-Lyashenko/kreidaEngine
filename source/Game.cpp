#include "Game.h"

#include <cmath>

bool Game::Init()
{
  eng::Engine& engine = eng::Engine::GetInstance();

  m_renderer = engine.GetRenderer2D();

  m_texture = engine.Assets().LoadTexture2D("assets/textures/Run.png");

  if (m_texture)
  {
    m_spriteSheet = std::make_unique<eng::SpriteSheet>(
        m_texture,
        128,
        128,
        0,
        0,
        64.0f,    // pivotX
        112.0f);  // pivotY

    std::vector<eng::Sprite> idleFrames = m_spriteSheet->GetSpritesInRow(0, 0, 4);

    std::vector<eng::Sprite> shotFrames = m_spriteSheet->GetSpritesInRow(
        0,  // row
        0,  // first column
        12  // frame count
    );

    m_shotClip = eng::AnimationClip(
        std::move(shotFrames),
        8.0f,  // FPS
        true);

    m_player.SetClip(&m_shotClip);
  }

  return m_renderer->IsValid() && m_texture.get()->IsValid() && m_spriteSheet.get()->IsValid();
}

void Game::Update(float DeltaTime)
{
  if (!m_renderer)
  {
    return;
  }

  m_player.Update(DeltaTime);

  if (m_player.HasClip())
  {
    m_renderer->DrawSpritePivoted(m_player.CurrentSprite(), 300.0f, 300.0f, 2.0f);
  }
}

void Game::Destroy()
{
  m_player.Stop();

  m_spriteSheet.reset();
  m_texture.reset();

  m_renderer = nullptr;
}
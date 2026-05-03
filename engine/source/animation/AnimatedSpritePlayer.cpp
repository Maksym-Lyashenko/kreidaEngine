#include "animation/AnimatedSpritePlayer.h"

#include <stdexcept>

namespace eng
{

void AnimatedSpritePlayer::SetClip(const AnimationClip* clip)
{
  m_clip = clip;
  Reset();

  if (m_clip != nullptr && m_clip->IsValid())
  {
    Play();
  }
}

void AnimatedSpritePlayer::Play()
{
  if (m_clip == nullptr || !m_clip->IsValid())
  {
    return;
  }

  m_playing = true;
  m_finished = false;
}

void AnimatedSpritePlayer::Pause()
{
  m_playing = false;
}

void AnimatedSpritePlayer::Stop()
{
  m_playing = false;
  Reset();
}

void AnimatedSpritePlayer::Reset()
{
  m_accumulator = 0.0f;
  m_currentFrame = 0;
  m_finished = false;
}

void AnimatedSpritePlayer::Update(float deltaTime)
{
  if (!m_playing || m_finished)
  {
    return;
  }

  if (m_clip == nullptr || !m_clip->IsValid())
  {
    return;
  }

  if (deltaTime <= 0.0f)
  {
    return;
  }

  const float frameDuration = m_clip->FrameDuration();

  if (frameDuration <= 0.0f)
  {
    return;
  }

  m_accumulator += deltaTime;

  while (m_accumulator >= frameDuration)
  {
    m_accumulator -= frameDuration;
    AdvanceFrame();

    if (m_finished)
    {
      break;
    }
  }
}

bool AnimatedSpritePlayer::IsPlaying() const
{
  return m_playing;
}

bool AnimatedSpritePlayer::HasClip() const
{
  return m_clip != nullptr && m_clip->IsValid();
}

bool AnimatedSpritePlayer::IsFinished() const
{
  return m_finished;
}

std::uint32_t AnimatedSpritePlayer::CurrentFrameIndex() const
{
  return m_currentFrame;
}

const Sprite& AnimatedSpritePlayer::CurrentSprite() const
{
  if (m_clip == nullptr || !m_clip->IsValid())
  {
    throw std::runtime_error("AnimatedSpritePlayer has no valid clip.");
  }

  return m_clip->Frame(m_currentFrame);
}

void AnimatedSpritePlayer::AdvanceFrame()
{
  if (m_clip == nullptr || !m_clip->IsValid())
  {
    return;
  }

  const std::uint32_t frameCount = m_clip->FrameCount();

  if (frameCount == 0)
  {
    return;
  }

  if (m_currentFrame + 1 < frameCount)
  {
    ++m_currentFrame;
    return;
  }

  if (m_clip->IsLooping())
  {
    m_currentFrame = 0;
  }
  else
  {
    m_currentFrame = frameCount - 1;
    m_finished = true;
    m_playing = false;
  }
}

}  // namespace eng
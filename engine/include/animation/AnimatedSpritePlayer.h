#pragma once

#include "animation/AnimationClip.h"

#include <cstdint>

namespace eng
{

class AnimatedSpritePlayer final
{
 public:
  AnimatedSpritePlayer() = default;

  void SetClip(const AnimationClip* clip);
  void Play();
  void Pause();
  void Stop();
  void Reset();

  void Update(float deltaTime);

  [[nodiscard]] bool IsPlaying() const;
  [[nodiscard]] bool HasClip() const;
  [[nodiscard]] bool IsFinished() const;

  [[nodiscard]] std::uint32_t CurrentFrameIndex() const;
  [[nodiscard]] const Sprite& CurrentSprite() const;

 private:
  void AdvanceFrame();

 private:
  const AnimationClip* m_clip = nullptr;

  float m_accumulator = 0.0f;

  std::uint32_t m_currentFrame = 0;

  bool m_playing = false;
  bool m_finished = false;
};

}  // namespace eng
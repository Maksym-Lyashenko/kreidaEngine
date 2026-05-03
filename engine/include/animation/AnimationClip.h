#pragma once

#include "sprites/SpriteSheet.h"

#include <cstdint>
#include <vector>

namespace eng
{

class AnimationClip final
{
 public:
  AnimationClip() = default;

  AnimationClip(std::vector<Sprite> frames, float framesPerSecond, bool looping = true);

  [[nodiscard]] bool IsValid() const;
  [[nodiscard]] bool IsLooping() const;

  [[nodiscard]] float FramesPerSecond() const;
  [[nodiscard]] float FrameDuration() const;

  [[nodiscard]] std::uint32_t FrameCount() const;

  [[nodiscard]] const Sprite& Frame(std::uint32_t index) const;

 private:
  std::vector<Sprite> m_frames;
  float m_framesPerSecond = 10.0f;
  bool m_looping = true;
};

}  // namespace eng
#include "animation/AnimationClip.h"

#include <stdexcept>
#include <utility>

namespace eng
{

AnimationClip::AnimationClip(std::vector<Sprite> frames, float framesPerSecond, bool looping)
    : m_frames(std::move(frames)), m_framesPerSecond(framesPerSecond), m_looping(looping)
{
}

bool AnimationClip::IsValid() const
{
  return !m_frames.empty() && m_framesPerSecond > 0.0f;
}

bool AnimationClip::IsLooping() const
{
  return m_looping;
}

float AnimationClip::FramesPerSecond() const
{
  return m_framesPerSecond;
}

float AnimationClip::FrameDuration() const
{
  if (m_framesPerSecond <= 0.0f)
  {
    return 0.0f;
  }

  return 1.0f / m_framesPerSecond;
}

std::uint32_t AnimationClip::FrameCount() const
{
  return static_cast<std::uint32_t>(m_frames.size());
}

const Sprite& AnimationClip::Frame(std::uint32_t index) const
{
  if (index >= m_frames.size())
  {
    throw std::runtime_error("AnimationClip frame index is out of range.");
  }

  return m_frames[index];
}

}  // namespace eng
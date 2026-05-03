#pragma once

#include <cstddef>
#include <vulkan/vulkan.h>

namespace eng::vk
{

struct Vertex2D final
{
  float position[2];

  static VkVertexInputBindingDescription BindingDescription()
  {
    VkVertexInputBindingDescription desc{};
    desc.binding = 0;
    desc.stride = sizeof(Vertex2D);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
  }

  static VkVertexInputAttributeDescription PositionAttributeDescription()
  {
    VkVertexInputAttributeDescription desc{};
    desc.location = 0;
    desc.binding = 0;
    desc.format = VK_FORMAT_R32G32_SFLOAT;
    desc.offset = offsetof(Vertex2D, position);
    return desc;
  }
};

}  // namespace eng::vk
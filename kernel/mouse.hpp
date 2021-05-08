#pragma once

#include "graphics.hpp"

class MouseCursor {
 public:
  MouseCursor(PixelWriter* writer, const FrameBufferConfig* fbConfig,
              PixelColor erase_color, Vector2D<int> initial_position);
  void MoveRelative(Vector2D<int> displacement);

 private:
  PixelWriter* writer = nullptr;
  const FrameBufferConfig* fbConfig;
  PixelColor erase_color;
  Vector2D<int> position;
};

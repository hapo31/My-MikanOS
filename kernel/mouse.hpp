#pragma once

#include "graphics.hpp"

class MouseCursor {
 public:
  MouseCursor(PixelWriter* writer, FrameBufferConfig* fbConfig,
              PixelColor erase_color, Vector2D<int> initial_position);
  void MoveRelative(Vector2D<int> displacement);

 private:
  PixelWriter* writer = nullptr;
  FrameBufferConfig* fbConfig;
  PixelColor erase_color;
  Vector2D<int> position;
};

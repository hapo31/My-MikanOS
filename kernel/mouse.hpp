#pragma once

#include "graphics.hpp"

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const PixelColor kMouseTransparentColor{0, 0, 1};

void DrawMouseCursor(PixelWriter& writer, Vector2D<int> position);
void InitializeMouse();

class Mouse {
 public:
  Mouse(unsigned int layer_id_) : layer_id(layer_id_) {}
  void OnInterrupt(uint8_t buttons, int8_t displacement_x, int displacement_y);

  unsigned int LayerID() const { return layer_id; }
  void SetPosition(Vector2D<int> position);
  Vector2D<int> Position() const { return position; }

 private:
  unsigned int layer_id;
  Vector2D<int> position{};
  unsigned int drag_layer_id{0};
  uint8_t previous_buttons{0};
};

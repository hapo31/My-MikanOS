#include "mouse.hpp"

namespace {
const char mouse_cousor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ", "@@             ", "@.@            ", "@..@           ",
    "@...@          ", "@....@         ", "@.....@        ", "@......@       ",
    "@.......@      ", "@........@     ", "@.........@    ", "@..........@   ",
    "@...........@  ", "@............@ ", "@......@@@@@@@@", "@......@       ",
    "@....@@.@      ", "@...@ @.@      ", "@..@   @.@     ", "@.@    @.@     ",
    "@@      @.@    ", "@       @.@    ", "         @.@   ", "         @@@   ",
};
}

void DrawMouseCursor(PixelWriter* writer, Vector2D<int> position) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cousor_shape[dy][dx] == '@') {
        writer->Write({0, 0, 0}, position.x + dx, position.y + dy);
      } else if (mouse_cousor_shape[dy][dx] == '.') {
        writer->Write({255, 255, 255}, position.x + dx, position.y + dy);
      }
    }
  }
}

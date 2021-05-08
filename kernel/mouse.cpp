#include "mouse.hpp"

namespace {

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;

const char mouse_cousor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ", "@@             ", "@.@            ", "@..@           ",
    "@...@          ", "@....@         ", "@.....@        ", "@......@       ",
    "@.......@      ", "@........@     ", "@.........@    ", "@..........@   ",
    "@...........@  ", "@............@ ", "@......@@@@@@@@", "@......@       ",
    "@....@@.@      ", "@...@ @.@      ", "@..@   @.@     ", "@.@    @.@     ",
    "@@      @.@    ", "@       @.@    ", "         @.@   ", "         @@@   ",
};

void DrawMouseCursor(PixelWriter* writer, const FrameBufferConfig* config,
                     Vector2D<int> position) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cousor_shape[dy][dx] == '@') {
        writer->Write(config, {0, 0, 0}, position.x + dx, position.y + dy);
      } else if (mouse_cousor_shape[dy][dx] == '.') {
        writer->Write(config, {255, 255, 255}, position.x + dx,
                      position.y + dy);
      }
    }
  }
}

void EraseMouseCursor(PixelWriter* writer, const FrameBufferConfig* config,
                      Vector2D<int> position, PixelColor erase_color) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cousor_shape[dy][dx] != ' ') {
        writer->Write(config, erase_color, position.x + dx, position.y + dy);
      }
    }
  }
}

}  // namespace

MouseCursor::MouseCursor(PixelWriter* writer_,
                         const FrameBufferConfig* fbConfig_,
                         PixelColor erase_color_,
                         Vector2D<int> initial_position_)
    : writer(writer_),
      fbConfig(fbConfig_),
      erase_color(erase_color_),
      position(initial_position_) {
  DrawMouseCursor(writer, fbConfig, initial_position_);
}

void MouseCursor::MoveRelative(Vector2D<int> displacement) {
  EraseMouseCursor(writer, fbConfig, position, erase_color);
  position += displacement;
  DrawMouseCursor(writer, fbConfig, position);
}

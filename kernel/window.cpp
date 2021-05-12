#include "window.hpp"

Window::Window(int width_, int height_) : width(width_), height(height_) {
  data.resize(height);
  for (int y = 0; y < height; ++y) {
    data[y].resize(width);
  }
}

void Window::DrawTo(PixelWriter& writer, Vector2D<int> position) {
  if (!transparent_color) {
    for (int y = 0; y < Height(); ++y) {
      for (int x = 0; x < Width(); ++x) {
        writer.Write(At(position.x + x, position.y + y), x, y);
      }
    }
    return;
  }

  const auto tc = transparent_color.value();
  for (int y = 0; y < Height(); ++y) {
    for (int x = 0; x < Width(); ++x) {
      const auto c = At(x, y);
      if (c != tc) {
        writer.Write(c, position.x + x, position.y + y);
      }
    }
  }
}

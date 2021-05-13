#include "window.hpp"

#include "logger.hpp"

Window::Window(int width_, int height_, PixelFormat shadow_format)
    : width{width_}, height{height_} {
  data.resize(height);
  for (int y = 0; y < height; ++y) {
    data[y].resize(width);
  }

  FrameBufferConfig config{};
  config.frame_buffer = nullptr;
  config.horizontal_resolution = width;
  config.vertical_resolution = height;
  config.pixel_format = shadow_format;

  if (auto err = shadow_buffer.Initialize(config)) {
    Log(kError, "failed to initialize shadow buffer: %s at %s:%d\n", err.Name(),
        err.File(), err.Line());
  }
}

void Window::DrawTo(FrameBuffer& dest, Vector2D<int> position) {
  if (!transparent_color) {
    dest.Copy(position, shadow_buffer);
    return;
  }

  const auto tc = transparent_color.value();
  auto& writer = dest.Writer();
  for (int y = 0; y < Height(); ++y) {
    for (int x = 0; x < Width(); ++x) {
      const auto c = At(x, y);
      if (c != tc) {
        writer.Write(c, position + Vector2D<int>{x, y});
      }
    }
  }
}

void Window::Write(const PixelColor& c, int x, int y) {
  data[y][x] = c;
  shadow_buffer.Writer().Write(c, x, y);
}

void Window::Move(Vector2D<int> dest_pos, const Rectangle<int>& src) {
  shadow_buffer.Move(dest_pos, src);
}
#include "window.hpp"

#include "font.hpp"
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
    Log(kError, "failed to initialize shadow_buffer: %s at %s:%d\n", err.Name(),
        err.File(), err.Line());
  }
}

void Window::DrawTo(FrameBuffer& dest, Vector2D<int> pos,
                    const Rectangle<int>& area) {
  if (!transparent_color) {
    Rectangle<int> window_area{pos, Size()};
    Rectangle<int> intersection = area & window_area;
    dest.Copy(intersection.pos, shadow_buffer,
              {intersection.pos - pos, intersection.size});
    return;
  }

  const auto tc = transparent_color.value();
  auto& writer = dest.Writer();
  for (int y = std::max(0, 0 - pos.y);
       y < std::min(Height(), writer.Height() - pos.y); ++y) {
    for (int x = std::max(0, 0 - pos.x);
         x < std::min(Width(), writer.Width() - pos.x); ++x) {
      const auto c = At(x, y);
      if (c != tc) {
        writer.Write(c, pos + Vector2D<int>{x, y});
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

namespace {

const int kCloseButtonWidth = 16;
const int kCloseButtonHeight = 14;
const char close_button[kCloseButtonHeight][kCloseButtonWidth + 1] = {
#include "CloseButton.csv"
};

constexpr PixelColor ToColor(uint32_t c) {
  return {static_cast<uint8_t>((c >> 16) & 0xff),
          static_cast<uint8_t>((c >> 8) & 0xff),
          static_cast<uint8_t>(c & 0xff)};
}

}  // namespace

void DrawWindow(PixelWriter& writer, const char* title) {
  auto fill_rect = [&writer](Vector2D<int> pos, Vector2D<int> size,
                             uint32_t c) {
    FillRect(writer, pos, size, ToColor(c));
  };

  const auto win_w = writer.Width();
  const auto win_h = writer.Height();

  fill_rect({0, 0}, {win_w, 1}, 0xc6c6c6);
  fill_rect({1, 1}, {win_w - 2, 1}, 0xffffff);
  fill_rect({0, 0}, {1, win_h}, 0xc6c6c6);
  fill_rect({1, 1}, {1, win_h - 2}, 0xffffff);
  fill_rect({win_w - 2, 1}, {1, win_h - 2}, 0x848484);
  fill_rect({win_w - 1, 0}, {1, win_h}, 0x000000);
  fill_rect({2, 2}, {win_w - 4, win_h - 4}, 0xc6c6c6);
  fill_rect({3, 3}, {win_w - 6, 18}, 0x000084);
  fill_rect({1, win_h - 2}, {win_w - 2, 1}, 0x848484);
  fill_rect({0, win_h - 1}, {win_w, 1}, 0x000000);

  WriteString(writer, 24, 4, ToColor(0xffffff), title);

  for (int y = 0; y < kCloseButtonHeight; ++y) {
    for (int x = 0; x < kCloseButtonWidth; ++x) {
      PixelColor c = ToColor(0xffffff);
      if (close_button[y][x] == '@') {
        c = ToColor(0x000000);
      } else if (close_button[y][x] == '$') {
        c = ToColor(0x848484);
      } else if (close_button[y][x] == ':') {
        c = ToColor(0xc6c6c6);
      }

      writer.Write(c, {win_w - 5 - kCloseButtonWidth + x, 5 + y});
    }
  }
}

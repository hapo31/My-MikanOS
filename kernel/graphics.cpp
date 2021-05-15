#include "./graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(const PixelColor& color, int x,
                                           int y) {
  auto p = GetPixel(x, y);
  p[0] = color.r;
  p[1] = color.g;
  p[2] = color.b;
}

void BGRResv8BitPerColorPixelWriter::Write(const PixelColor& color, int x,
                                           int y) {
  auto p = GetPixel(x, y);
  p[0] = color.b;
  p[1] = color.g;
  p[2] = color.r;
}

void DrawRect(PixelWriter& writer, const Vector2D<int>& pos,
              const Vector2D<int>& size, const PixelColor& color) {
  for (int dx = 0; dx < size.x; ++dx) {
    writer.Write(color, pos.x + dx, pos.y);
    writer.Write(color, pos.x + dx, pos.y + size.y - 1);
  }
  for (int dy = 1; dy < size.y - 1; ++dy) {
    writer.Write(color, pos.x, pos.y + dy);
    writer.Write(color, pos.x + size.x - 1, pos.y + dy);
  }
}

void FillRect(PixelWriter& writer, const Vector2D<int>& pos,
              const Vector2D<int>& size, const PixelColor& color) {
  for (int dy = 0; dy < size.y; ++dy) {
    for (int dx = 0; dx < size.x; ++dx) {
      writer.Write(color, dx + pos.x, dy + pos.y);
    }
  }
}

void DrawDesktop(PixelWriter& writer) {
  int width = writer.Width();
  int height = writer.Height();
  FillRect(writer, {0, 0}, {width, height - 50}, kDesktopBGColor);
  FillRect(writer, {0, height - 50}, {width, 50}, {1, 8, 17});
  FillRect(writer, {0, height - 50}, {width / 5, 50}, {80, 80, 80});
  DrawRect(writer, {10, height - 40}, {30, 30}, {160, 160, 160});
}

PixelWriter* screen_writer;
FrameBufferConfig screen_config;

Vector2D<int> ScreenSize() {
  return {static_cast<int>(screen_config.horizontal_resolution),
          static_cast<int>(screen_config.vertical_resolution)};
}

namespace {
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];

PixelWriter* CreateFrameWriter(const FrameBufferConfig* config) {
  switch (config->pixel_format) {
    case kPixelRGBResv8BitPerColor:
      return new (pixel_writer_buf) RGBResv8BitPerColorPixelWriter(*config);
    case kPixelBGRResv8BitPerColor:
      return new (pixel_writer_buf) BGRResv8BitPerColorPixelWriter(*config);

    default:
      exit(1);
  }
}
}  // namespace

void InitializeGraphics(const FrameBufferConfig& config) {
  ::screen_config = config;

  ::screen_writer = CreateFrameWriter(&config);
  DrawDesktop(*screen_writer);
}

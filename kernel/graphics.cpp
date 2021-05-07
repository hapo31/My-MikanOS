#include "./graphics.hpp"

uint8_t* PixelWriter::GetPixel(const FrameBufferConfig* config, int x, int y) {
  const int pixel_position = config->pixels_per_scan_line * y + x;
  return &config->frame_buffer[4 * pixel_position];
}

void RGBResv8BitPerColorPixelWriter::Write(uint8_t** target, const PixelColor& color) {
  (*target)[0] = color.r;
  (*target)[1] = color.g;
  (*target)[2] = color.b;
}

void BGRResv8BitPerColorPixelWriter::Write(uint8_t** target, const PixelColor& color) {
  (*target)[0] = color.b;
  (*target)[1] = color.g;
  (*target)[2] = color.r;
}

void FillRect(PixelWriter* writer,
              const FrameBufferConfig* config,
              const Vector2D<int>& pos,
              const Vector2D<int>& size,
              const PixelColor& color) {

  for (int dy = 0; dy < size.y; ++dy) {
    for (int dx = 0; dx < size.x; ++dx) {
      auto target = writer->GetPixel(config, dx + pos.x, dy + pos.y);
      writer->Write(&target, color);
    }
  }
}

void DrawRect(PixelWriter* writer,
              const FrameBufferConfig* config,
              const Vector2D<int>& pos,
              const Vector2D<int>& size,
              const PixelColor& color) {
  for (int dx = 0; dx < size.x; ++dx) {
    auto target = writer->GetPixel(config, pos.x + dx, pos.y);
    writer->Write(&target, color);
    target = writer->GetPixel(config, pos.x + dx, pos.y + size.y - 1);
    writer->Write(&target, color);
  }
  for (int dy = 0; dy < size.x; ++dy) {
    auto target = writer->GetPixel(config, pos.x, pos.y + dy);
    writer->Write(&target, color);
    target = writer->GetPixel(config, pos.x + size.x, pos.y + dy);
    writer->Write(&target, color);
  }
}

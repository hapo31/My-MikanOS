#include "./graphics.hpp"

uint8_t* PixelWriter::GetPixel(const FrameBufferConfig& config, int x, int y) {
  const int pixel_position = config.pixels_per_scan_line * y + x;
  return &config.frame_buffer[4 * pixel_position];
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

#define _LIBCPP_HAS_NO_THREADS
#include <cstdint>

#include "frame_buffer.hpp"

struct PixelColor {
  uint8_t r, g, b;
};

int WritePixel(const FrameBufferConfig* config, int x, int y, const PixelColor& c) {
  const int pixel_position = config->pixels_per_scan_line * y + x;
  if (config->pixel_format == kPixelRGBResv8BitPerColor) {
    uint8_t *p = &config->frame_buffer[4 * pixel_position];
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  } else if (config->pixel_format == kPixelBGRResv8BitPerColor) {
    uint8_t *p = &config->frame_buffer[4 * pixel_position];
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
  } else {
    return -1;
  }

  return 0;
}

extern "C" void KernelMain(const struct FrameBufferConfig* config) {
  uint8_t *frame_buffer = reinterpret_cast<uint8_t *>(config->frame_buffer);

  for (uint64_t y = 0; y < config->vertical_resolution; ++y) {
    for (uint64_t x = 0; x < config->hozontal_resolution; ++x) {
      WritePixel(config, x, y, {255, 255, 255});
    }
  }
  while (1)
    __asm__("hlt");
}

#pragma once

#include <stdint.h>

struct PixelColor {
  uint8_t r, g, b;
};

enum PixelFormat
{
  kPixelRGBResv8BitPerColor,
  kPixelBGRResv8BitPerColor,
};

struct FrameBufferConfig { 
  uint8_t* frame_buffer;
  uint32_t pixels_per_scan_line;
  uint32_t hozontal_resolution;
  uint32_t vertical_resolution;
  enum PixelFormat pixel_format;
};

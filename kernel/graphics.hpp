#pragma once
#include "frame_buffer.hpp"

struct PixelColor {
  uint8_t r, g, b;
};

class PixelWriter {
public:
  PixelWriter() = default;
  virtual ~PixelWriter() = default;
  uint8_t *GetPixel(const FrameBufferConfig &config, int x, int y);
  virtual void Write(uint8_t **target, const PixelColor &color) = 0;
};

class RGBResv8BitPerColorPixelWriter: public PixelWriter {
public:
  using PixelWriter::PixelWriter;
  ~RGBResv8BitPerColorPixelWriter() = default;
  virtual void Write(uint8_t **target, const PixelColor &color) override;
};

class BGRResv8BitPerColorPixelWriter: public PixelWriter {
public:
  using PixelWriter::PixelWriter;
  ~BGRResv8BitPerColorPixelWriter() = default;
  virtual void Write(uint8_t **target, const PixelColor &color) override;
};

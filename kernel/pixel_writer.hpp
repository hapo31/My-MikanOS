#pragma once
#include "frame_buffer.hpp"

class PixelWriter {
public:
  PixelWriter() = default;
  virtual ~PixelWriter() = default;
  virtual void Write(uint8_t** target, const PixelColor& color) = 0;
};

class RGBResv8BitPerColorPixelWriter: public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  ~RGBResv8BitPerColorPixelWriter() = default;

  virtual void Write(uint8_t** target, const PixelColor& color) override {
    (*target)[0] = color.r;
    (*target)[1] = color.g;
    (*target)[2] = color.b;
  }
};

class BGRResv8BitPerColorPixelWriter: public PixelWriter {
public:
  using PixelWriter::PixelWriter;

  ~BGRResv8BitPerColorPixelWriter() = default;

  virtual void Write(uint8_t** target, const PixelColor& color) override {
    (*target)[0] = color.b;
    (*target)[1] = color.g;
    (*target)[2] = color.r;
  }
};

#pragma once
#include "frame_buffer.hpp"

struct PixelColor {
  uint8_t r, g, b;
};

template <typename T>
struct Vector2D {
  T x, y;

  template <typename U>
  Vector2D<T> &operator+=(const Vector2D<U> &rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
};

class PixelWriter {
 public:
  PixelWriter() = default;
  virtual ~PixelWriter() = default;
  uint8_t *GetPixel(const FrameBufferConfig *config, int x, int y);
  virtual void Write(uint8_t **target, const PixelColor &color) = 0;
  virtual void Write(const FrameBufferConfig *config, const PixelColor &color,
                     int x, int y) {
    auto target = GetPixel(config, x, y);
    Write(&target, color);
  }
};

class RGBResv8BitPerColorPixelWriter : public PixelWriter {
 public:
  using PixelWriter::PixelWriter;
  ~RGBResv8BitPerColorPixelWriter() = default;
  virtual void Write(uint8_t **target, const PixelColor &color) override;
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
 public:
  using PixelWriter::PixelWriter;
  ~BGRResv8BitPerColorPixelWriter() = default;
  virtual void Write(uint8_t **target, const PixelColor &color) override;
};

void FillRect(PixelWriter *writer, const FrameBufferConfig *config,
              const Vector2D<int> &pos, const Vector2D<int> &size,
              const PixelColor &color);

void DrawRect(PixelWriter *writer, const FrameBufferConfig *config,
              const Vector2D<int> &pos, const Vector2D<int> &size,
              const PixelColor &color);

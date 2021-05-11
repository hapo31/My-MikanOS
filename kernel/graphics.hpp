#pragma once
#include "frame_buffer.hpp"

struct PixelColor {
  uint8_t r, g, b;
};

inline bool operator==(const PixelColor &lhs, const PixelColor &rhs) {
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

inline bool operator!=(const PixelColor &lhs, const PixelColor &rhs) {
  return !(lhs == rhs);
}

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
  virtual ~PixelWriter() = default;
  virtual void Write(const PixelColor &color, int x, int y) = 0;
  virtual int Width() const = 0;
  virtual int Height() const = 0;
};

class FrameBufferWriter : public PixelWriter {
 public:
  FrameBufferWriter(const FrameBufferConfig &fbConfig_) : fbConfig(fbConfig_) {}
  ~FrameBufferWriter() = default;
  virtual int Width() const override { return fbConfig.horizontal_resolution; }
  virtual int Height() const override { return fbConfig.vertical_resolution; }

 protected:
  uint8_t *GetPixel(int x, int y) {
    return fbConfig.frame_buffer + 4 * (fbConfig.pixels_per_scan_line * y + x);
  }

 private:
  const FrameBufferConfig &fbConfig;
};

class RGBResv8BitPerColorPixelWriter : public FrameBufferWriter {
 public:
  using FrameBufferWriter::FrameBufferWriter;
  virtual void Write(const PixelColor &color, int x, int y) override;
};

class BGRResv8BitPerColorPixelWriter : public FrameBufferWriter {
 public:
  using FrameBufferWriter::FrameBufferWriter;
  virtual void Write(const PixelColor &color, int x, int y) override;
};

void FillRect(PixelWriter &writer, const Vector2D<int> &pos,
              const Vector2D<int> &size, const PixelColor &color);

void DrawRect(PixelWriter &writer, const Vector2D<int> &pos,
              const Vector2D<int> &size, const PixelColor &color);

const PixelColor kDesktopBGColor{100, 128, 50};
const PixelColor kDesktopFGColor{0, 255, 255};

void DrawDesktop(PixelWriter &writer);

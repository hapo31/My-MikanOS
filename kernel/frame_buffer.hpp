#pragma once

#include <memory>
#include <vector>

#include "error.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"

class FrameBuffer {
 public:
  Error Initialize(const FrameBufferConfig& conifg);
  Error Copy(Vector2D<int> pos, const FrameBuffer& src);

  FrameBufferWriter& Writer() { return *writer; }

 private:
  FrameBufferConfig config{};
  std::vector<uint8_t> buffer{};
  std::unique_ptr<FrameBufferWriter> writer{};
};

int BitsPerPixel(PixelFormat format);

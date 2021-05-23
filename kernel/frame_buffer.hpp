#pragma once

#include <memory>
#include <vector>

#include "error.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"

class FrameBuffer {
 public:
  Error Initialize(const FrameBufferConfig& conifg);
  Error CopyFrom(const FrameBuffer& src, Vector2D<int> pos,
                 const Rectangle<int>& src_area);
  void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);

  FrameBufferWriter& Writer() { return *writer; }
  const FrameBufferConfig& Config() const { return config; }

 private:
  FrameBufferConfig config{};
  std::vector<uint8_t> buffer{};
  std::unique_ptr<FrameBufferWriter> writer{};
};

int BitsPerPixel(PixelFormat format);

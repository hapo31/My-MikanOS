#include "frame_buffer.hpp"

#include "logger.hpp"

Error FrameBuffer::Initialize(const FrameBufferConfig& config) {
  this->config = config;

  const auto bits_per_pixel = BitsPerPixel(config.pixel_format);
  if (bits_per_pixel <= 0) {
    return MAKE_ERROR(Error::kUnknownPixelFormat);
  }

  if (config.frame_buffer) {
    buffer.resize(0);
  } else {
    buffer.resize(((bits_per_pixel + 7) / 8) *
                  this->config.horizontal_resolution *
                  this->config.vertical_resolution);

    this->config.frame_buffer = buffer.data();
    this->config.pixels_per_scan_line = this->config.horizontal_resolution;
  }

  Log(kInfo, "width:%u, height:%u pixels_per_scan_line:%u\n",
      this->config.horizontal_resolution, this->config.vertical_resolution,
      this->config.pixels_per_scan_line);

  switch (this->config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
      writer = std::make_unique<RGBResv8BitPerColorPixelWriter>(this->config);
      break;
    case kPixelBGRResv8BitPerColor:
      writer = std::make_unique<BGRResv8BitPerColorPixelWriter>(this->config);
      break;

    default:
      return MAKE_ERROR(Error::kUnknownPixelFormat);
  }

  return MAKE_ERROR(Error::kSuccess);
}

Error FrameBuffer::Copy(Vector2D<int> pos, const FrameBuffer& src) {
  if (config.pixel_format != src.config.pixel_format) {
    return MAKE_ERROR(Error::kUnknownPixelFormat);
  }

  const auto bits_per_pixel = BitsPerPixel(config.pixel_format);
  if (bits_per_pixel <= 0) {
    return MAKE_ERROR(Error::kUnknownPixelFormat);
  }

  const auto dst_width = config.horizontal_resolution;
  const auto dst_height = config.vertical_resolution;
  const auto src_width = src.config.horizontal_resolution;
  const auto src_height = src.config.vertical_resolution;

  const int copy_start_dst_x = std::max(pos.x, 0);
  const int copy_start_dst_y = std::max(pos.y, 0);
  const int copy_end_dst_x = std::min(pos.x + src_width, dst_width);
  const int copy_end_dst_y = std::min(pos.y + src_height, dst_height);

  const auto bytes_per_pixel = (bits_per_pixel + 7) / 8;
  const auto bytes_per_copy_line =
      bytes_per_pixel * (copy_end_dst_x - copy_start_dst_x);

  uint8_t* dst_buf =
      config.frame_buffer +
      bytes_per_pixel *
          (config.pixels_per_scan_line * copy_start_dst_y + copy_start_dst_x);

  const uint8_t* src_buf = src.config.frame_buffer;

  for (int dy = 0; dy < copy_end_dst_y - copy_start_dst_y; ++dy) {
    memcpy(dst_buf, src_buf, bytes_per_copy_line);
    dst_buf += bytes_per_pixel * config.pixels_per_scan_line;
    src_buf += bytes_per_pixel * src.config.pixels_per_scan_line;
  }

  return MAKE_ERROR(Error::kSuccess);
}

int BitsPerPixel(PixelFormat format) {
  switch (format) {
    case kPixelRGBResv8BitPerColor:
      return 32;
    case kPixelBGRResv8BitPerColor:
      return 32;
  }

  return -1;
}

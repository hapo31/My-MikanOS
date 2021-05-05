#define _LIBCPP_HAS_NO_THREADS
#include <cstdint>
#include <cstddef>

#include "frame_buffer.hpp"
#include "pixel_writer.hpp"

void* operator new(size_t size, void* buf) {
  return buf;
}

void operator delete(void* obj) noexcept {
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

PixelWriter* CreatePixelContext(const struct FrameBufferConfig* config) {
  switch (config->pixel_format) {
    case kPixelRGBResv8BitPerColor:
      return pixel_writer = new(pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter;
      break;

    case kPixelBGRResv8BitPerColor:
      return pixel_writer = new(pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter;
      break;
    }
}

int WritePixel(PixelWriter* writer, const FrameBufferConfig *config, int x, int y, const PixelColor &c) {
  const int pixel_position = config->pixels_per_scan_line * y + x;
  uint8_t *p = &config->frame_buffer[4 * pixel_position];
  writer->Write(&p, c);
  return 0;
}

extern "C" void KernelMain(const struct FrameBufferConfig* config) {
  uint8_t *frame_buffer = reinterpret_cast<uint8_t *>(config->frame_buffer);

  auto writer = CreatePixelContext(config);

  for (uint64_t y = 0; y < config->vertical_resolution; ++y) {
    for (uint64_t x = 0; x < config->hozontal_resolution; ++x) {
      WritePixel(writer, config, x, y, {255, 255, 255});
    }
  }

  while (1)
    __asm__("hlt");
}

#include "font.hpp"

extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

void WriteAscii(PixelWriter &writer, int x, int y, const PixelColor &color,
                char c) {
  const uint8_t *font = GetFont(c);

  if (font == nullptr) {
    return;
  }

  for (int dy = 0; dy < 16; ++dy) {
    for (int dx = 0; dx < 8; ++dx) {
      if (font[dy] << dx & 0x80u) {
        writer.Write(color, dx + x, dy + y);
      }
    }
  }
}

void WriteString(PixelWriter &writer, int x, int y, const PixelColor &color,
                 const char *str) {
  for (int i = 0; str[i] != '\0'; ++i) {
    WriteAscii(writer, x + i * 8, y, color, str[i]);
  }
}

const uint8_t *GetFont(char c) {
  auto index = 16 * static_cast<unsigned int>(c);

  if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
    return nullptr;
  }

  return &_binary_hankaku_bin_start + index;
}

#pragma once

#include <cstdint>

#include "./graphics.hpp"

void WriteAscii(PixelWriter &writer, int x, int y, const PixelColor &color,
                char c);
void WriteString(PixelWriter &writer, int x, int y, const PixelColor &color,
                 const char *c);

const uint8_t *GetFont(char c);

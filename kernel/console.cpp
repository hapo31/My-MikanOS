#include "console.hpp"
#include "font.hpp"
#include <cstring>

Console::Console(PixelWriter *writer_, const FrameBufferConfig *config_, const PixelColor &fgColor_, const PixelColor &bgColor_):
  writer(writer_),
  fbConfig(config_),
  fgColor(fgColor_), 
  bgColor(bgColor_),
  buffer{}, 
  cursorRow(0), 
  cursorColumns(0)
{
}

void Console::PutString(const char* str) {
  while(*str) {
    if (*str == '\n') {
      NewLine();
    } else if (cursorColumns < kColumns - 1) {
      WriteAscii(writer, fbConfig, 8 * cursorColumns, 16 * cursorRow, fgColor, *str);
      buffer[cursorRow][cursorColumns] = *str;
      ++cursorColumns;
    }
    ++str;
  }
}

void Console::NewLine() {
  cursorColumns = 0;
  if (cursorRow < kRows - 1) {
    ++cursorRow;
  } else {
    for (int y = 0; y < 16 * kRows; ++y) {
      for (int x = 0; x < 8 * kColumns; ++x) {
        auto target = writer->GetPixel(fbConfig, x, y);
        writer->Write(&target, bgColor);
      }
    }

    for (int row = 0; row < kRows - 1; ++row) {
      memcpy(buffer[row], buffer[row + 1], kColumns + 1);
      WriteString(writer, fbConfig, 0, 16 * row, fgColor, buffer[row]);
    }
    memset(buffer[kRows - 1], 0, kColumns + 1);
  }
}

#include "console.hpp"

#include <cstring>

#include "font.hpp"

Console::Console(PixelWriter *writer_, const PixelColor &fgColor_,
                 const PixelColor &bgColor_)
    : writer(writer_),
      fgColor(fgColor_),
      bgColor(bgColor_),
      buffer{},
      cursorRow(0),
      cursorColumns(0) {}

void Console::PutString(const char *str) {
  while (*str) {
    if (*str == '\n') {
      NewLine();
    } else if (cursorColumns < kColumns - 1) {
      WriteAscii(writer, 8 * cursorColumns, 16 * cursorRow, fgColor, *str);
      buffer[cursorRow][cursorColumns] = *str;
      ++cursorColumns;
    }
    ++str;
  }
}

void Console::SetWriter(PixelWriter *writer) {
  if (this->writer == writer) {
    return;
  }

  this->writer = writer;
  Refresh();
}

void Console::NewLine() {
  cursorColumns = 0;
  if (cursorRow < kRows - 1) {
    ++cursorRow;
  } else {
    for (int y = 0; y < 16 * kRows; ++y) {
      for (int x = 0; x < 8 * kColumns; ++x) {
        writer->Write(bgColor, x, y);
      }
    }

    for (int row = 0; row < kRows - 1; ++row) {
      memcpy(buffer[row], buffer[row + 1], kColumns + 1);
      WriteString(writer, 0, 16 * row, fgColor, buffer[row]);
    }
    memset(buffer[kRows - 1], 0, kColumns + 1);
  }
}

void Console::Refresh() {
  for (int row = 0; row < kRows; ++row) {
    WriteString(writer, 0, 16 * row, fgColor, buffer[row]);
  }
}
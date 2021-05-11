#pragma once

#include "graphics.hpp"

class Console {
 public:
  static const int kRows = 30, kColumns = 80;
  Console(PixelWriter *writer, const PixelColor &fgColor,
          const PixelColor &bgColor);
  void PutString(const char *a);
  void SetWriter(PixelWriter *writer);

 private:
  void NewLine();
  void Refresh();
  PixelWriter *writer;
  const PixelColor fgColor, bgColor;
  char buffer[kRows][kColumns + 1];
  int cursorRow;
  int cursorColumns;
};

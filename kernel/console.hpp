#pragma once
#include <memory>

#include "graphics.hpp"
#include "window.hpp"

class Console {
 public:
  static const int kRows = 30, kColumns = 80;
  Console(const PixelColor &fgColor, const PixelColor &bgColor);
  void PutString(const char *a);
  void SetWriter(PixelWriter *writer);
  void SetWindow(const std::shared_ptr<Window> &window);

 private:
  void NewLine();
  void Refresh();
  PixelWriter *writer;
  const PixelColor fgColor, bgColor;
  char buffer[kRows][kColumns + 1];
  int cursorRow;
  int cursorColumns;

  std::shared_ptr<Window> window;
};

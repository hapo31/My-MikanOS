#pragma once
#include <memory>

#include "graphics.hpp"
#include "window.hpp"

class Console {
 public:
  static const int kRows = 30, kColumns = 80;
  Console(const PixelColor &fgColor, const PixelColor &bgColor);
  void PutString(const char *a);
  void SetWriter(const std::shared_ptr<PixelWriter> &writer);
  void SetLayerID(unsigned int id) { layer_id = id; }
  unsigned int LayerID() const;

 private:
  void NewLine();
  void Refresh();
  std::shared_ptr<PixelWriter> writer;
  const PixelColor fgColor, bgColor;
  char buffer[kRows][kColumns + 1];
  int cursorRow;
  int cursorColumns;
  unsigned int layer_id;

  friend int printk(const char *format, ...);
};

const PixelColor kConsoleCharColor{200, 200, 200};

extern Console *console;

void InitializeConsole();

int printk(const char *format, ...);
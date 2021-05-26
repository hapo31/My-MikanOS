#pragma once
#include <memory>

#include "graphics.hpp"
#include "window.hpp"

class Console {
 public:
  static const int kRows = 30, kColumns = 80;
  Console(const PixelColor &fgColor, const PixelColor &bgColor);
  void PutString(const char *a);
  void SetWriter(PixelWriter &writer);
  void SetLayerID(unsigned int id) { layer_id = id; }
  unsigned int LayerID() const;

 protected:
  virtual void NewLine();
  void Refresh();
  PixelWriter *writer;
  const PixelColor fgColor, bgColor;
  char buffer[kRows][kColumns + 1];
  int cursorRow;
  int cursorColumns;
  unsigned int layer_id;

  friend int printk(const char *format, ...);
};

class WindowConsole : public Console {
 public:
  using Console::Console;
  WindowConsole(const PixelColor &fgColor, const PixelColor &bgColor,
                const std::shared_ptr<ToplevelWindow> &window);
  void SetWindow(const std::shared_ptr<ToplevelWindow> &window);
  void NewLine() override;

 private:
  std::shared_ptr<ToplevelWindow> window;
};

const auto kConsoleBGColor = ToColor(0x2C001E);
const auto kConsoleCharColor = ToColor(0xEEEEEE);

extern Console *direct_console;
extern WindowConsole *console;

void InitializeDirectConsole();
void InitializeWindowConsole();

int printk(const char *format, ...);
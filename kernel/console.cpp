#include "console.hpp"

#include <cstdio>
#include <cstring>

#include "font.hpp"
#include "layer.hpp"

Console::Console(const PixelColor &fgColor_, const PixelColor &bgColor_)
    : writer(nullptr),
      fgColor(fgColor_),
      bgColor(bgColor_),
      buffer{},
      cursorRow(0),
      cursorColumns(0) {}

void Console::PutString(const char *str) {
  while (*str) {
    if (*str == '\n') {
      NewLine();
    } else if (cursorColumns < kColumns) {
      WriteAscii(*writer, 8 * cursorColumns, 16 * cursorRow, fgColor, *str);
      buffer[cursorRow][cursorColumns] = *str;
      ++cursorColumns;
      if (cursorColumns >= kColumns) {
        NewLine();
      }
    }
    ++str;
  }
  if (layer_manager) {
    layer_manager->Draw(this->layer_id);
  }
}

void Console::SetWriter(PixelWriter &writer) {
  if (this->writer == &writer) {
    return;
  }

  this->writer = &writer;
  Refresh();
}

void Console::NewLine() {
  cursorColumns = 0;
  if (cursorRow < kRows - 1) {
    ++cursorRow;
    return;
  }

  FillRect(*writer, {0, 0}, {8 * kColumns, 16 * kRows}, bgColor);
  for (int row = 0; row < kRows - 1; ++row) {
    memcpy(buffer[row], buffer[row + 1], kColumns + 1);
    WriteString(*writer, 0, 16 * row, fgColor, buffer[row]);
  }
  memset(buffer[kRows - 1], 0, kColumns + 1);
}

void Console::Refresh() {
  FillRect(*writer, {0, 0}, {8 * kColumns, 16 * kRows}, bgColor);
  for (int row = 0; row < kRows; ++row) {
    WriteString(*writer, 0, 16 * row, fgColor, buffer[row]);
  }
}

WindowConsole::WindowConsole(const PixelColor &fgColor,
                             const PixelColor &bgColor,
                             const std::shared_ptr<ToplevelWindow> &window_)
    : Console(fgColor, bgColor), window(window_) {
  SetWriter(window->InnerWriter());
}

void WindowConsole::SetWindow(const std::shared_ptr<ToplevelWindow> &window) {
  if (this->window == window) {
    return;
  }
  this->writer = &window->InnerWriter();
  this->window = window;
  Refresh();
}

void WindowConsole::NewLine() {
  cursorColumns = 0;
  if (cursorRow < kRows - 1) {
    ++cursorRow;
    return;
  }

  Rectangle<int> move_src{{0, 16}, {8 * kColumns, 16 * (kRows - 1)}};
  window->Move({0, 0}, move_src);
  FillRect(*writer, {0, 16 * (kRows - 1)}, {8 * kColumns, 16}, bgColor);
}

Console *direct_console;
WindowConsole *console;

namespace {
char direct_console_buf[sizeof(Console)];
char window_console_buf[sizeof(WindowConsole)];
}  // namespace

void InitializeDirectConsole() {
  direct_console =
      new (direct_console_buf) Console(kConsoleCharColor, kDesktopBGColor);
  direct_console->SetWriter(*::screen_writer.get());
}

void InitializeWindowConsole() {
  auto console_window = std::make_shared<ToplevelWindow>(
      Console::kColumns * 8, Console::kRows * 16, screen_config.pixel_format,
      "Terminal");
  console = new (window_console_buf)
      WindowConsole(kConsoleCharColor, kConsoleBGColor, console_window);

  auto console_layer_id = layer_manager->NewLayer()
                              .SetWindow(console_window)
                              .SetDraggable(true)
                              .Move({0, 0})
                              .ID();

  console->SetLayerID(console_layer_id);
  layer_manager->UpDown(console_layer_id, 1);
}

int printk(const char *format, ...) {
  va_list ap;
  int result;
  char s[1024];
  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);
  direct_console->PutString(s);
  return result;
}

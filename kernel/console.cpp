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
    } else if (cursorColumns < kColumns - 1) {
      WriteAscii(*writer, 8 * cursorColumns, 16 * cursorRow, fgColor, *str);
      buffer[cursorRow][cursorColumns] = *str;
      ++cursorColumns;
    }
    ++str;
  }
  if (layer_manager) {
    layer_manager->Draw(this->layer_id);
  }
}

void Console::SetWriter(PixelWriter *writer) {
  if (this->writer == writer) {
    return;
  }

  this->writer = writer;
  window.reset();
  Refresh();
}

void Console::SetWindow(const std::shared_ptr<Window> &window) {
  if (this->window == window) {
    return;
  }
  this->window = window;
  writer = window.get();
  Refresh();
}

void Console::NewLine() {
  cursorColumns = 0;
  if (cursorRow < kRows - 1) {
    ++cursorRow;
    return;
  }

  if (window) {
    Rectangle<int> move_src{{0, 16}, {8 * kColumns, 16 * (kRows - 1)}};
    window->Move({0, 0}, move_src);
    FillRect(*writer, {0, 16 * (kRows - 1)}, {8 * kColumns, 16}, bgColor);
  } else {
    FillRect(*writer, {0, 0}, {8 * kColumns, 16 * kRows}, bgColor);
    for (int row = 0; row < kRows - 1; ++row) {
      memcpy(buffer[row], buffer[row + 1], kColumns + 1);
      WriteString(*writer, 0, 16 * row, fgColor, buffer[row]);
    }
    memset(buffer[kRows - 1], 0, kColumns + 1);
  }
}

void Console::Refresh() {
  for (int row = 0; row < kRows; ++row) {
    WriteString(*writer, 0, 16 * row, fgColor, buffer[row]);
  }
}

Console *console;

namespace {
char console_buf[sizeof(Console)];
}  // namespace

void InitializeConsole() {
  console = new (console_buf) Console(kConsoleCharColor, kDesktopBGColor);
  console->SetWriter(::screen_writer);
}

int printk(const char *format, ...) {
  va_list ap;
  int result;
  char s[1024];
  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);
  console->PutString(s);
  return result;
}

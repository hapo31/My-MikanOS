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

void Console::SetWriter(const std::shared_ptr<PixelWriter> &writer) {
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

  if (layer_manager) {
    layer_manager->Draw(layer_id);
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
  auto src_writer = console->writer;
  console->SetWriter(::screen_writer);
  va_list ap;
  int result;
  char s[1024];
  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);
  console->PutString(s);

  console->SetWriter(src_writer);
  return result;
}

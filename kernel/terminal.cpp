#include "terminal.hpp"

#include <string.h>

#include "layer.hpp"
#include "pci.hpp"
#include "task.hpp"

Terminal::Terminal() {
  window = std::make_shared<ToplevelWindow>(
      kColumns * 8 + ToplevelWindow::kMarginX,
      kRows * 16 + 8 + ToplevelWindow::kMarginY, screen_config.pixel_format,
      "MS-DOS");

  DrawTerminal(window->InnerWriter(), {0, 0}, window->InnerSize());

  layer_id =
      layer_manager->NewLayer().SetWindow(window).SetDraggable(true).ID();

  Print(">");
  cmd_history.resize(16);
}

Rectangle<int> Terminal::InputKey(uint8_t modifier, uint8_t keycode,
                                  char ascii) {
  DrawCursor(false);

  Rectangle<int> draw_area{CalcCursorPos(), {8 * 2, 16}};
  if (ascii == '\n') {
    line_buf[linebuf_index] = 0;
    if (linebuf_index > 0) {
      cmd_history.pop_back();
      cmd_history.emplace_front(line_buf);
    }
    linebuf_index = 0;
    cmd_history_index = -1;
    cursor.x = 0;
    Log(kWarn, "line: %s\n", &line_buf[0]);
    if (cursor.y < kRows - 1) {
      ++cursor.y;
    } else {
      Scroll();
    }
    ExecuteLine();
    Print(">");
    draw_area.pos = ToplevelWindow::kTopLeftMargin;
    draw_area.size = window->InnerSize();
  } else if (ascii == '\b') {
    if (cursor.x > 0) {
      --cursor.x;
      FillRect(*window, CalcCursorPos(), {8, 16}, {0, 0, 0});
      draw_area.pos = CalcCursorPos();

      if (linebuf_index > 0) {
        --linebuf_index;
      }
    }
  } else if (ascii != 0) {
    if (cursor.x < kColumns - 1 && linebuf_index < kLineMax - 1) {
      line_buf[linebuf_index] = ascii;
      ++linebuf_index;
      auto pos = CalcCursorPos();
      WriteAscii(*window, pos.x, pos.y, {255, 255, 255}, ascii);
      ++cursor.x;
    }
  } else if (keycode == 0x51) {
    draw_area = HistoryUpDown(-1);
  } else if (keycode == 0x52) {
    draw_area = HistoryUpDown(1);
  }

  DrawCursor(true);

  return draw_area;
}

void Terminal::Print(const char* str) {
  DrawCursor(false);

  auto newline = [this]() {
    cursor.x = 0;
    if (cursor.y < kRows - 1) {
      ++cursor.y;
    } else {
      Scroll();
    }
  };

  while (*str) {
    if (*str == '\n') {
      newline();
    } else {
      WriteAscii(*window, CalcCursorPos(), {255, 255, 255}, *str);
      if (cursor.x == kColumns - 1) {
        newline();
      } else {
        ++cursor.x;
      }
    }

    ++str;
  }

  DrawCursor(true);
}

void Terminal::ExecuteLine() {
  char* command = &line_buf[0];

  char* first_arg = strchr(command, ' ');
  if (first_arg) {
    *first_arg = 0;
    ++first_arg;
  }

  if (strncmp(command, "echo", 4) == 0) {
    if (first_arg) {
      Print(first_arg);
    }
    Print("\n");
  } else if (strncmp(command, "clear", 5) == 0) {
    FillRect(window->InnerWriter(), {4, 4}, {8 * kColumns, 16 * kRows},
             {0, 0, 0});
    cursor.y = 0;
  } else if (strncmp(command, "lspci", 5) == 0) {
    char s[64];
    for (int i = 0; i < pci::num_devices; ++i) {
      const auto& dev = pci::devices[i];
      auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
      sprintf(s, "%02x:%02x.%d vend=%04x head=%02x class=%02x.%02x.%02x\n",
              dev.bus, dev.device, dev.function, vendor_id, dev.header_type,
              dev.class_code.base, dev.class_code.sub,
              dev.class_code.interface);
      Print(s);
    }
  } else if (command[0] != 0) {
    Print("no such command: ");
    Print(command);
    Print("\n");
  }
}

void Terminal::BlinkCursor() {
  cursor_visible = !cursor_visible;
  DrawCursor(cursor_visible);
}

void Terminal::DrawCursor(bool visible) {
  const auto color = visible ? ToColor(0xffffff) : ToColor(0);
  const auto pos = Vector2D<int>{4 + 8 * cursor.x, 5 + 16 * cursor.y};
  FillRect(window->InnerWriter(), pos, {7, 15}, color);
}

Vector2D<int> Terminal::CalcCursorPos() const {
  return ToplevelWindow::kTopLeftMargin +
         Vector2D<int>{4 + 8 * cursor.x, 4 + 16 * cursor.y};
}

void Terminal::Scroll() {
  Rectangle<int> move_src{{4, 4 + 16}, {8 * kColumns, 16 * (kRows - 1)}};
  window->Move({4, 4}, move_src);
  FillRect(window->InnerWriter(), {4, 4 + 16 * cursor.y}, {8 * kColumns, 16},
           {0, 0, 0});
}

Rectangle<int> Terminal::HistoryUpDown(int direction) {
  if (direction == -1 && cmd_history_index >= 0) {
    --cmd_history_index;
  } else if (direction == 1 && cmd_history_index + 1 < cmd_history.size()) {
    ++cmd_history_index;
  }

  cursor.x = 1;
  const auto first_pos = CalcCursorPos();
  Rectangle<int> draw_area{first_pos, {8 * (kColumns - 1), 16}};
  FillRect(*window, draw_area.pos, draw_area.size, {0, 0, 0});

  const char* history = "";
  if (cmd_history_index >= 0) {
    history = &cmd_history[cmd_history_index][0];
  }

  strcpy(&line_buf[0], history);
  linebuf_index = strlen(history);

  WriteString(*window, first_pos, {255, 255, 255}, history);
  cursor.x = linebuf_index + 1;
  return draw_area;
}

void TaskTerminal(uint64_t task_id, int64_t data) {
  asm("cli");
  Task& task = task_manager->CurrentTask();
  Terminal* terminal = new Terminal;
  layer_manager->Move(terminal->LayerID(), {150, 200});
  active_layer->Activate(terminal->LayerID());
  layer_task_map->emplace(std::make_pair(terminal->LayerID(), task_id));
  asm("sti");

  while (true) {
    asm("cli");
    auto msg = task.ReceiveMessage();
    if (!msg) {
      task.Sleep();
      asm("sti");
      continue;
    }

    switch (msg->type) {
      case Message::kKeyPush: {
        const auto area = terminal->InputKey(msg->arg.keyboard.modifier,
                                             msg->arg.keyboard.keycode,
                                             msg->arg.keyboard.ascii);
        Message msg = MakeLayerMessage(task_id, terminal->LayerID(),
                                       LayerOperation::DrawArea, area);
        __asm__("cli");
        task_manager->SendMessage(1, msg);
        __asm__("sti");
        break;
      }

      case Message::kTimerTimeout:
        terminal->BlinkCursor();
        {
          auto area = terminal->CursorArea();
          auto msg = MakeLayerMessage(task_id, terminal->LayerID(),
                                      LayerOperation::DrawArea, area);
          asm("cli");
          task_manager->SendMessage(1, msg);
          asm("sti");
        }
        break;
      default:
        break;
    }
  }
}

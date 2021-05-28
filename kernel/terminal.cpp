#include "terminal.hpp"

#include "layer.hpp"
#include "task.hpp"

Terminal::Terminal() {
  window = std::make_shared<ToplevelWindow>(
      kColumns * 8 + ToplevelWindow::kMarginX,
      kRows * 16 + 8 + ToplevelWindow::kMarginY, screen_config.pixel_format,
      "MS-DOS");

  DrawTerminal(window->InnerWriter(), {0, 0}, window->InnerSize());

  layer_id =
      layer_manager->NewLayer().SetWindow(window).SetDraggable(true).ID();
}

Rectangle<int> Terminal::InputKey(uint8_t modifier, uint8_t keycode,
                                  char ascii) {
  DrawCursor(false);

  Rectangle<int> draw_area{CalcCursorPos(), {8 * 2, 16}};
  if (ascii == '\n') {
    line_buf[linebuf_index] = 0;
    linebuf_index = 0;
    cursor.x = 0;
    Log(kWarn, "line: %s\n", &line_buf[0]);
    if (cursor.y < kRows - 1) {
      ++cursor.y;
    } else {
      Scroll();
    }
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
  }

  DrawCursor(true);

  return draw_area;
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
  Rectangle<int> move_src{
      ToplevelWindow::kTopLeftMargin + Vector2D<int>{4, 4 + 16},
      {8 * kColumns, 16 * (kRows - 1)}};

  window->Move(ToplevelWindow::kTopLeftMargin + Vector2D<int>{4, 4}, move_src);
  FillRect(window->InnerWriter(), {4, 4 + 16 * cursor.y}, {8 * kColumns, 16},
           {0, 0, 0});
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

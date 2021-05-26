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

void Terminal::BlinkCursor() {
  cursor_visible = !cursor_visible;
  DrawCursor(cursor_visible);
}

void Terminal::DrawCursor(bool visible) {
  const auto color = visible ? ToColor(0xffffff) : ToColor(0);
  const auto pos = Vector2D<int>{4 + 8 * cursor.x, 5 + 16 * cursor.y};
  FillRect(window->InnerWriter(), pos, {7, 15}, color);
}

void TaskTerminal(uint64_t task_id, int64_t data) {
  asm("cli");
  Task& task = task_manager->CurrentTask();
  Terminal* terminal = new Terminal;
  layer_manager->Move(terminal->LayerID(), {150, 200});
  active_layer->Activate(terminal->LayerID());
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
      case Message::kTimerTimeout:
        terminal->BlinkCursor();
        {
          Message msg{Message::kLayer, task_id};
          msg.arg.layer.layer_id = terminal->LayerID();
          msg.arg.layer.op = LayerOperation::Draw;
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

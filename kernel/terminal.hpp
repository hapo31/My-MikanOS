#pragma once
#include <memory>

#include "graphics.hpp"
#include "window.hpp"

class Terminal {
 public:
  static const int kRows = 15, kColumns = 60;

  Terminal();
  unsigned int LayerID() const { return layer_id; }
  void BlinkCursor();
  Rectangle<int> CursorArea() const {
    return {ToplevelWindow::kTopLeftMargin +
                Vector2D<int>{4 + 8 * cursor.x, 5 + 16 * cursor.y},
            {7, 15}};
  }

 private:
  std::shared_ptr<ToplevelWindow> window;
  unsigned int layer_id;
  Vector2D<int> cursor{0, 0};
  bool cursor_visible{false};
  void DrawCursor(bool visible);
};

void TaskTerminal(uint64_t task_id, int64_t data);

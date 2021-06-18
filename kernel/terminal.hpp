#pragma once
#include <deque>
#include <memory>

#include "graphics.hpp"
#include "window.hpp"

class Terminal {
 public:
  static const int kRows = 15, kColumns = 60;
  static const int kLineMax = 128;
  Terminal();
  unsigned int LayerID() const { return layer_id; }
  void BlinkCursor();
  Rectangle<int> CursorArea() const {
    return {ToplevelWindow::kTopLeftMargin +
                Vector2D<int>{4 + 8 * cursor.x, 5 + 16 * cursor.y},
            {7, 15}};
  }

  Rectangle<int> InputKey(uint8_t modifier, uint8_t keycode, char ascii);

  void Print(char c);
  void Print(const char* str);
  void ExecuteLine();

 private:
  std::shared_ptr<ToplevelWindow> window;
  unsigned int layer_id;
  Vector2D<int> cursor{0, 0};
  bool cursor_visible{false};
  void DrawCursor(bool visible);

  Vector2D<int> CalcCursorPos() const;

  int linebuf_index{0};
  std::array<char, kLineMax> line_buf{};
  std::deque<std::array<char, kLineMax>> cmd_history{};
  int cmd_history_index{-1};
  Rectangle<int> HistoryUpDown(int direction);
  void Scroll();
};

void TaskTerminal(uint64_t task_id, int64_t data);

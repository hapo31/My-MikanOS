#define _LIBCPP_HAS_NO_THREADS
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <limits>
#include <numeric>
#include <vector>

#include "acpi.hpp"
#include "asmfunc.h"
#include "benchmark.hpp"
#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
#include "keyboard.hpp"
#include "layer.hpp"
#include "logger.hpp"
#include "memory_manager.hpp"
#include "memory_map.hpp"
#include "message.hpp"
#include "mouse.hpp"
#include "paging.hpp"
#include "pci.hpp"
#include "queue.hpp"
#include "segment.hpp"
#include "task.hpp"
#include "terminal.hpp"
#include "timer.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"

class ScopedLock {
 public:
  ScopedLock() { asm("cli"); }
  ~ScopedLock() { asm("sti"); }
};

void Lock() { asm("cli"); }

void Free() { asm("sti"); }

void operator delete(void *obj) noexcept {}

void DrawTextCursor(bool visible);

void Exit() {
  while (true) asm("hlt");
}

std::shared_ptr<ToplevelWindow> main_window;
unsigned int main_window_layer_id;

void InitializeMainWindow() {
  main_window = std::make_shared<ToplevelWindow>(
      160, 30, screen_config.pixel_format, "fuckOS!");
  main_window_layer_id = layer_manager->NewLayer()
                             .SetWindow(main_window)
                             .SetDraggable(true)
                             .Move({300, 100})
                             .ID();

  layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

std::shared_ptr<ToplevelWindow> text_window;
unsigned int text_window_layer_id;

void InitializeTextWindow() {
  const int win_w = 160;
  const int win_h = 30;

  text_window = std::make_shared<ToplevelWindow>(
      win_w, win_h, screen_config.pixel_format, "Text Box Test(Text dakeni)");

  DrawTextBox(text_window->InnerWriter(), {0, 0}, text_window->InnerSize());
  DrawTextCursor(true);
  text_window_layer_id = layer_manager->NewLayer()
                             .SetWindow(text_window)
                             .SetDraggable(true)
                             .Move({250, 200})
                             .ID();

  layer_manager->UpDown(text_window_layer_id, std::numeric_limits<int>::max());
}

int text_window_index;

void DrawTextCursor(bool visible) {
  const auto color = visible ? ToColor(0) : ToColor(0xffffff);
  const auto pos = Vector2D<int>{4 + 8 * text_window_index, 5};
  FillRect(text_window->InnerWriter(), pos, {7, 15}, color);
}

void InputTextWindow(char c) {
  if (c == 0) {
    return;
  }

  auto pos = []() { return Vector2D<int>{4 + 8 * text_window_index, 6}; };
  const int max_chars = (text_window->InnerSize().x - 8) / 8;
  auto &writer = text_window->InnerWriter();
  if (c == '\b' && text_window_index > 0) {
    DrawTextCursor(false);
    --text_window_index;
    FillRect(writer, pos(), {8, 16}, ToColor(0xffffff));
    DrawTextCursor(true);
  } else if (c >= ' ' && text_window_index < max_chars) {
    DrawTextCursor(false);
    const auto p = pos();
    WriteAscii(writer, p.x, p.y, ToColor(0), c);
    ++text_window_index;
    DrawTextCursor(true);
  }

  layer_manager->Draw(text_window_layer_id);
}

const int kCellSize = 5;
const auto kCellColor = ToColor(0xe4007f);
const auto kFieldColor = ToColor(0x0000ff);
int field_width;
int field_height;

std::shared_ptr<ToplevelWindow> lifegame_window;
unsigned int lifegame_window_layer_id;

std::vector<int> field;

uint32_t getRand(void) {
  static uint32_t y = 1284002006;
  y = y ^ (y << 13);
  y = y ^ (y >> 17);
  return y = y ^ (y << 5);
}

void InitializeLifeGame(int width, int height) {
  const int win_h = kCellSize * (height + 1);
  const int win_w = kCellSize * (width + 1);

  field_width = width;
  field_height = height;

  field.resize((height + 2) * (width + 2));
  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      field[y * width + x] = (getRand() % 3) >= 1;
    }
  }

  lifegame_window = std::make_shared<ToplevelWindow>(
      win_w, win_h, screen_config.pixel_format, "life game");

  FillRect(lifegame_window->InnerWriter(), {1, 1}, {win_w, win_h}, kFieldColor);

  lifegame_window_layer_id = layer_manager->NewLayer()
                                 .SetWindow(lifegame_window)
                                 .SetDraggable(true)
                                 .Move({400, 0})
                                 .ID();

  layer_manager->UpDown(lifegame_window_layer_id,
                        std::numeric_limits<int>::max());
}

void UpdateLifeGame(uint64_t task_id, int64_t data) {
  static uint gen = 0;

  auto field_color_bytes = (data >> 32) & 0xffff;
  auto cell_color_bytes = (data)&0xffff;

  auto field_color = ToColor(field_color_bytes);
  auto cell_color = ToColor(cell_color_bytes);

  auto &task = task_manager->CurrentTask();

  while (true) {
    {
      std::vector<int> nextField(field);
      for (int y = 1; y < field_height - 1; ++y) {
        for (int x = 1; x < field_width - 1; ++x) {
          int live = 0;
          // 左上
          if (field[(y - 1) * field_width + x - 1]) {
            live++;
          }
          // 上
          if (field[(y - 1) * field_width + x]) {
            live++;
          }
          // 右上
          if (field[(y - 1) * field_width + x + 1]) {
            live++;
          }
          // 右
          if (field[y * field_width + x + 1]) {
            live++;
          }
          // 右下
          if (field[(y + 1) * field_width + x + 1]) {
            live++;
          }
          // 下
          if (field[(y + 1) * field_width + x]) {
            live++;
          }
          // 左下
          if (field[(y + 1) * field_width + x - 1]) {
            live++;
          }
          // 左
          if (field[y * field_width + x - 1]) {
            live++;
          }
          if (live == 2 && field[y * field_width + x] || live == 3) {
            nextField[y * field_width + x] = 1;
          } else {
            nextField[y * field_width + x] = 0;
          }
        }
      }
      field = nextField;
    }
    const int win_h = kCellSize * field_height;
    const int win_w = kCellSize * field_width;
    FillRect(lifegame_window->InnerWriter(), {1, 1}, {win_w, win_h},
             field_color);
    for (int y = 0; y < field_height; ++y) {
      for (int x = 0; x < field_width; ++x) {
        if (field[y * field_width + x]) {
          FillRect(lifegame_window->InnerWriter(),
                   {(x - 1) * kCellSize, (y - 1) * kCellSize},
                   {kCellSize, kCellSize}, cell_color);
        }
      }
    }

    char buf[32];
    sprintf(buf, "%d", gen);
    WriteString(lifegame_window->InnerWriter(), 0, 0, ToColor(0xff0000), buf);

    gen++;

    Message msg{Message::kLayer, task_id};
    msg.arg.layer.layer_id = lifegame_window_layer_id;
    msg.arg.layer.op = LayerOperation::Draw;
    asm("cli");
    task_manager->SendMessage(1, msg);
    asm("sti");

    while (true) {
      asm("cli");
      auto msg = task.ReceiveMessage();
      if (!msg) {
        task.Sleep();
        asm("sti");
        continue;
      }
      if (msg->type == Message::kLayerFinish) {
        break;
      }
    }
  }
}

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig &config_ref,
                                   const MemoryMap &memmap_ref,
                                   const acpi::RSDP &acpi_table) {
  const FrameBufferConfig config{config_ref};
  const MemoryMap memmap{memmap_ref};

  const auto screen_size = ScreenSize();

  InitializeGraphics(config);
  InitializeDirectConsole();

  /*
    ログレベルの設定
    kInfo: それっぽいやつだけ
    kDebug: 全部
    kError: エラー
  */
  SetLogLevel(kInfo);

  printk("Welcome to fuckOS!\n");
  printk("Display info: %dx%d\n", config.horizontal_resolution,
         config.vertical_resolution);

  InitializeSegmentation();
  InitializePaging();
  InitializeMemoryManager(memmap);
  InitializeInterrupt();

  InitializePCI();

  InitializeLayer();
  InitializeMainWindow();
  InitializeTextWindow();
  InitializeLifeGame(65, 40);

  layer_manager->Draw({{0, 0}, screen_size});
  layer_manager->Draw(1);

  acpi::Initialize(acpi_table);
  InitializeLAPICTimer();

  const int kTextBoxCursorTimer = 1;
  const int kTimer1Sec = static_cast<int>(kTimerFreq * 1);
  __asm__("cli");
  timer_manager->AddTimer(Timer{kTimer1Sec, kTextBoxCursorTimer});
  __asm__("sti");
  bool textbox_cursor_visible = false;

  InitializeTask();

  auto &main_task = task_manager->CurrentTask();

  const auto lifegame_taskid =
      task_manager->NewTask()
          .InitContext(UpdateLifeGame, 0xdeadbeefc0ffee)
          .Wakeup()
          .ID();

  const auto terminal_taskid =
      task_manager->NewTask().InitContext(TaskTerminal, 0).Wakeup().ID();

  InitializeBenchMark();

  const auto benchmark_taskid =
      task_manager->NewTask().InitContext(TaskBenchMark, 0).Wakeup().ID();

  usb::xhci::Initialize();
  InitializeKeyboard();
  InitializeMouse();

  active_layer->Activate(lifegame_window_layer_id);

  char str[128];

#pragma region メッセージループ

  for (;;) {
    __asm__("cli");
    const auto count = timer_manager->CurrentTick();
    __asm__("sti");

    sprintf(str, "%010lu", count);
    FillRect(main_window->InnerWriter(), {20, 4}, {8 * 10, 16},
             {0xc6, 0xc6, 0xc6});
    WriteString(main_window->InnerWriter(), 20, 4, {0, 0, 0}, str);
    layer_manager->Draw(main_window_layer_id);

    __asm__("cli");

    auto msg = main_task.ReceiveMessage();
    if (!msg) {
      main_task.Sleep();
      __asm__("sti");
      continue;
    }

    __asm__("sti");

    switch (msg->type) {
      case Message::kInterruptXHCI:
        usb::xhci::ProcessEvents();
        break;
      case Message::kTimerTimeout:
        if (msg->arg.timer.value == kTextBoxCursorTimer) {
          {
            ScopedLock lock;
            timer_manager->AddTimer(Timer{msg->arg.timer.timeout + kTimer1Sec,
                                          kTextBoxCursorTimer});
          }
          textbox_cursor_visible = !textbox_cursor_visible;
          DrawTextCursor(textbox_cursor_visible);
          layer_manager->Draw(text_window_layer_id);
          {
            ScopedLock lock;
            task_manager->SendMessage(terminal_taskid, *msg);
            task_manager->SendMessage(benchmark_taskid, *msg);
          }
        }

        break;
      case Message::kKeyPush:
        if (auto act = active_layer->GetActive();
            msg->arg.keyboard.ascii != 0 && act == text_window_layer_id) {
          InputTextWindow(msg->arg.keyboard.ascii);
        } else if (act == lifegame_window_layer_id) {
          if (msg->arg.keyboard.ascii == 's') {
            Log(kInfo, "sleep life\n");
            task_manager->Sleep(lifegame_taskid);
          } else if (msg->arg.keyboard.ascii == 'w') {
            task_manager->Wakeup(lifegame_taskid);
            Log(kInfo, "wakeup life\n");
          }
        } else {
          Lock();
          auto task_it = layer_task_map->find(act);
          Free();
          if (task_it != layer_task_map->end()) {
            Lock();
            task_manager->SendMessage(task_it->second, *msg);
            Free();
          } else {
            Log(kInfo, "key push not handle: Keycode %02x, ascii: %02x\n",
                msg->arg.keyboard.keycode, msg->arg.keyboard.ascii);
          }
        }
        break;

      case Message::kLayer:
        ProcessLayerMessage(*msg);
        {
          task_manager->SendMessage(msg->src_task,
                                    Message{Message::kLayerFinish});
        }
        break;

      default:
        Log(kError, "Unknown message type: %d\n", msg->type);
    }
  }

#pragma endregion

  while (1) __asm__("hlt");
}

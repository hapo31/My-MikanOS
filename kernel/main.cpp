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
#include "timer.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"
#include "window.hpp"

void operator delete(void *obj) noexcept {}

void DrawTextCursor(bool visible);

std::shared_ptr<Window> main_window;
unsigned int main_window_layer_id;

void InitializeMainWindow() {
  main_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
  DrawWindow(*main_window, "fuckOS!");

  main_window_layer_id = layer_manager->NewLayer()
                             .SetWindow(main_window)
                             .SetDraggable(true)
                             .Move({300, 100})
                             .ID();

  layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

std::shared_ptr<Window> text_window;
unsigned int text_window_layer_id;

void InitializeTextWindow() {
  const int win_w = 250;
  const int win_h = 52;

  text_window =
      std::make_shared<Window>(win_w, win_h, screen_config.pixel_format);
  DrawWindow(*text_window, "Text Box Test(Text dakeni)");
  DrawTextBox(*text_window, {4, 24}, {win_w - 8, win_h - 24 - 4});
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
  const auto pos = Vector2D<int>{8 + 8 * text_window_index, 24 + 5};
  FillRect(*text_window, pos, {7, 15}, color);
}

void InputTextWindow(char c) {
  if (c == 0) {
    return;
  }

  auto pos = []() { return Vector2D<int>{8 + 8 * text_window_index, 24 + 6}; };
  const int max_chars = (text_window->Width() - 16) / 8;
  if (c == '\b' && text_window_index > 0) {
    DrawTextCursor(false);
    --text_window_index;
    FillRect(*text_window, pos(), {8, 16}, ToColor(0xffffff));
    DrawTextCursor(true);
  } else if (c >= ' ' && text_window_index < max_chars) {
    DrawTextCursor(false);
    const auto p = pos();
    WriteAscii(*text_window, p.x, p.y, ToColor(0), c);
    ++text_window_index;
    DrawTextCursor(true);
  }

  layer_manager->Draw(text_window_layer_id);
}

struct TaskContext {
  uint64_t cr3, rip, rflags, reserved1;
  uint64_t cs, ss, fs, gs;
  uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rpb;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  std::array<uint8_t, 512> fxsave_area;
} __attribute__((packed));

alignas(16) TaskContext lifegame_context, main_context;

const int kCellSize = 5;
const auto kCellColor = ToColor(0xe4007f);
const auto kFieldColor = ToColor(0x0000ff);
const int height = 50;
const int width = 75;

std::shared_ptr<Window> lifegame_window;
unsigned int lifegame_window_layer_id;

std::vector<int> field;

uint32_t getRand(void) {
  static uint32_t y = 2493534442;
  y = y ^ (y << 13);
  y = y ^ (y >> 17);
  return y = y ^ (y << 5);
}

void InitializeLifeGame() {
  const int win_h = kCellSize * height;
  const int win_w = kCellSize * width;

  field.resize((height + 2) * (width + 2));
  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      field[y * width + x] = getRand() % 3;
    }
  }

  lifegame_window =
      std::make_shared<Window>(win_w, win_h, screen_config.pixel_format);

  FillRect(*lifegame_window, {1, 1}, {win_w - 2, win_h - 2}, kFieldColor);

  lifegame_window_layer_id = layer_manager->NewLayer()
                                 .SetWindow(lifegame_window)
                                 .SetDraggable(true)
                                 .Move({100, 200})
                                 .ID();

  layer_manager->UpDown(lifegame_window_layer_id,
                        std::numeric_limits<int>::max());
}

void UpdateLifeGame() {
  while (true) {
    {
      std::vector<int> nextField(field.size());
      for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
          int live = 0;
          // 左上
          if (field[(y - 1) * width + x - 1]) {
            live++;
          }
          // 上
          if (field[(y - 1) * width + x]) {
            live++;
          }
          // 右上
          if (field[(y - 1) * width + x + 1]) {
            live++;
          }
          // 右
          if (field[y * width + x + 1]) {
            live++;
          }
          // 右下
          if (field[(y + 1) * width + x + 1]) {
            live++;
          }
          // 下
          if (field[(y + 1) * width + x]) {
            live++;
          }
          // 左下
          if (field[(y + 1) * width + x - 1]) {
            live++;
          }
          // 左
          if (field[y * width + x - 1]) {
            live++;
          }
          if (live == 2 && field[y * width + x] || live == 3) {
            nextField[y * width + x] = 1;
          } else {
            nextField[y * width + x] = 0;
          }
        }
        field = std::move(nextField);
      }
    }
    const int win_h = kCellSize * height;
    const int win_w = kCellSize * width;
    FillRect(*lifegame_window, {1, 1}, {win_w - 2, win_h - 2}, kFieldColor);
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        if (field[y * width + x]) {
          FillRect(*lifegame_window, {x * kCellSize, y * kCellSize},
                   {kCellSize, kCellSize}, kCellColor);
        }
      }
    }

    layer_manager->Draw(lifegame_window_layer_id);

    SwitchContext(&main_context, &lifegame_context);
  }
}

std::deque<Message> *main_queue;

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig &config_ref,
                                   const MemoryMap &memmap_ref,
                                   const acpi::RSDP &acpi_table) {
  const FrameBufferConfig config{config_ref};
  const MemoryMap memmap{memmap_ref};

  const auto screen_size = ScreenSize();

  InitializeGraphics(config);
  InitializeConsole();

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
  ::main_queue = new std::deque<Message>(32);
  InitializeInterrupt(main_queue);

  InitializePCI();
  usb::xhci::Initialize();

  InitializeLayer();
  InitializeMainWindow();
  InitializeTextWindow();
  InitializeLifeGame();
  InitializeMouse();

  layer_manager->Draw({{0, 0}, screen_size});

  acpi::Initialize(acpi_table);
  InitializeLAPICTimer(*main_queue);
  InitializeKeyboard(*main_queue);

  const int kTextBoxCursorTimer = 1;
  const int kTimer1Sec = static_cast<int>(kTimerFreq * 1);
  __asm__("cli");
  timer_manager->AddTimer(Timer{kTimer1Sec, kTextBoxCursorTimer});
  __asm__("sti");
  bool textbox_cursor_visible = false;

  std::vector<uint64_t> lifegame_task_stack(1024);
  uint64_t lifegame_stack_end =
      reinterpret_cast<uint64_t>(&lifegame_task_stack[1024]);

  memset(&lifegame_task_stack, 0, sizeof(lifegame_context));
  lifegame_context.rip = reinterpret_cast<uint64_t>(UpdateLifeGame);
  lifegame_context.rdi = 1;
  lifegame_context.rsi = 42;

  lifegame_context.cr3 = GetCR3();
  lifegame_context.rflags = 0x202;
  lifegame_context.cs = kKernelCS;
  lifegame_context.ss = kKernelSS;
  lifegame_context.rsp = (lifegame_stack_end & ~0xflu) - 8;

  *reinterpret_cast<uint32_t *>(&lifegame_context.fxsave_area[24]) = 0x1f80;

  char str[128];

#pragma region メッセージループ

  for (;;) {
    auto count = timer_manager->CurrentTick();
    sprintf(str, "%010lu", count);
    FillRect(*main_window, {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
    WriteString(*main_window, 24, 28, {0, 0, 0}, str);
    layer_manager->Draw(1);
    layer_manager->Draw(main_window_layer_id);
    __asm__("cli");
    if (main_queue->size() == 0) {
      __asm__("sti");
      SwitchContext(&lifegame_context, &main_context);
      continue;
    }

    Message msg = main_queue->front();
    main_queue->pop_front();
    __asm__("sti");

    switch (msg.type) {
      case Message::kInterruptXHCI:
        usb::xhci::ProcessEvents();
        break;
      case Message::kTimerTimeout:
        if (msg.arg.timer.value == kTextBoxCursorTimer) {
          __asm__("cli");
          timer_manager->AddTimer(
              Timer{msg.arg.timer.timeout + kTimer1Sec, kTextBoxCursorTimer});
          __asm__("sti");
          textbox_cursor_visible = !textbox_cursor_visible;
          DrawTextCursor(textbox_cursor_visible);
          layer_manager->Draw(text_window_layer_id);
        }

        break;
      case Message::kKeyPush:
        if (msg.arg.keyboard.ascii != 0) {
          InputTextWindow(msg.arg.keyboard.ascii);
        }
        break;
      default:
        Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }

#pragma endregion

  while (1) __asm__("hlt");
}

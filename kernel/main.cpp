#define _LIBCPP_HAS_NO_THREADS
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <limits>
#include <numeric>
#include <vector>

#include "asmfunc.h"
#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
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

std::deque<Message> *main_queue;
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

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig &config_ref,
                                   const MemoryMap &memmap_ref) {
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
  InitializeMouse();

  layer_manager->Draw({{0, 0}, screen_size});

  InitializeLAPICTimer(*main_queue);
  timer_manager->AddTimer(Timer{200, 2});
  timer_manager->AddTimer(Timer(600, -1));

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
      __asm__("sti\n\thlt");
      continue;
    }

    Message msg = main_queue->front();
    main_queue->pop_front();
    __asm__("sti");

    switch (msg.type) {
      case Message::kInterruptXHCI:
        usb::xhci::ProcessEvents();
        break;
      case Message::kInterruptTimer:
        break;

      case Message::kTimerTimeout:
        printk("Timer timeout = %lu, value = %d\n", msg.arg.timer.timeout,
               msg.arg.timer.value);
        if (msg.arg.timer.value > 0) {
          timer_manager->AddTimer(
              Timer(msg.arg.timer.timeout + 100, msg.arg.timer.value + 1));
        }
        break;
      default:
        Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }

#pragma endregion

  while (1) __asm__("hlt");
}

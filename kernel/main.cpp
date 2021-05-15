#define _LIBCPP_HAS_NO_THREADS
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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

#pragma region ユーティリティ関数

struct Message {
  enum Type {
    kInterruptXHCI,
  } type;
};

void operator delete(void *obj) noexcept {}

char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager *memory_manager;

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

char console_buf[sizeof(Console)];
Console *console;

ArrayQueue<Message> *main_queue;

PixelWriter *CreateFrameWriter(const struct FrameBufferConfig *config) {
  switch (config->pixel_format) {
    case kPixelRGBResv8BitPerColor:
      return pixel_writer =
                 new (pixel_writer_buf) RGBResv8BitPerColorPixelWriter(*config);
      break;

    case kPixelBGRResv8BitPerColor:
      return pixel_writer =
                 new (pixel_writer_buf) BGRResv8BitPerColorPixelWriter(*config);
      break;
  }
}

usb::xhci::Controller *xhc;

__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame) {
  main_queue->Push(Message{Message::kInterruptXHCI});
  NotifyEndOfInterrupt();
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

unsigned int mouse_layer_id;
Vector2D<int> screen_size;
Vector2D<int> mouse_position;

void MouseObserver(uint8_t buttons, int8_t displacement_x,
                   int8_t displacement_y) {
  static unsigned int drag_layer_id = 0;
  static uint8_t previous_buttons = 0;

  const auto old_pos = mouse_position;
  auto new_pos = mouse_position + Vector2D<int>{displacement_x, displacement_y};
  new_pos = ElementMin(new_pos, screen_size + Vector2D<int>{-1, -1});
  mouse_position = ElementMax(new_pos, {0, 0});

  const auto pos_diff = mouse_position - old_pos;

  layer_manager->Move(mouse_layer_id, mouse_position);

  Log(kDebug, "MouseObserver: (%d,%d)\n", displacement_x, displacement_y);

  const bool previous_left_pressed = previous_buttons & 0x01;
  const bool left_pressed = buttons & 0x01;

  if (!previous_left_pressed && left_pressed) {
    auto layer =
        layer_manager->FindLayerByPosition(mouse_position, mouse_layer_id);
    if (layer && layer->IsDraggable()) {
      drag_layer_id = layer->ID();
    }
  } else if (previous_left_pressed && left_pressed) {
    if (drag_layer_id > 0) {
      layer_manager->MoveRelative(drag_layer_id, pos_diff);
    }
  } else if (previous_left_pressed && !left_pressed) {
    drag_layer_id = 0;
  }

  previous_buttons = buttons;
}

void SwitchEhci2Xhci(const pci::Device &xhcDev) {
  bool intel_ehc_exist = false;
  for (int i = 0; i < pci::num_devices; ++i) {
    const auto &dev = pci::devices[i];
    if (dev.class_code.Match(0x0cu, 0x03u, 0x20u) &&
        pci::ReadVendorId(dev) == 0x8086) {
      intel_ehc_exist = true;
      break;
    }
  }

  if (!intel_ehc_exist) {
    return;
  }

  uint32_t superspeed_ports = pci::ReadConfReg(xhcDev, 0xdc);
  pci::WriteConfReg(xhcDev, 0xd8, superspeed_ports);
  uint32_t ehci2xhci_ports = pci::ReadConfReg(xhcDev, 0xd4);
  pci::WriteConfReg(xhcDev, 0xd0, ehci2xhci_ports);
  Log(kDebug, "SwitchEhci2Xhci: SS = %02x, xHCI = %02x\n", superspeed_ports,
      ehci2xhci_ports);
}

std::string CreateErrorMessage(const std::string &message, const Error &err) {
  return message + ":" + err.Name() + " at " + err.File() + ":" +
         std::to_string(err.Line()) + "\n";
}

#pragma endregion

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig &config_ref,
                                   const MemoryMap &memmap_ref) {
  const FrameBufferConfig config{config_ref};
  const MemoryMap memmap{memmap_ref};

  screen_size = {static_cast<int>(config.horizontal_resolution),
                 static_cast<int>(config.vertical_resolution)};

  auto writer = CreateFrameWriter(&config);

  DrawDesktop(*pixel_writer);

  console = new (console_buf) Console({200, 200, 200}, kDesktopBGColor);

  console->SetWriter(pixel_writer);

  /*
    ログレベルの設定
    kInfo: それっぽいやつだけ
    kDebug: 全部
    kError: エラー
  */
  SetLogLevel(kDebug);

  printk("Welcome to fuckOS!\n");
  printk("Display info: %dx%d\n", config.horizontal_resolution,
         config.vertical_resolution);

#pragma region セグメントレジスタの初期化
  SetupSegments();

  const uint16_t kernel_cs = 1 << 3;
  const uint16_t kernel_ss = 2 << 3;

  SetDSAll(0);
  SetCSSS(kernel_cs, kernel_ss);

  SetupIdentityPageTable();

#pragma region メモリ管理初期化

  ::memory_manager = new (memory_manager_buf) BitmapMemoryManager;
  const auto memory_map_base = reinterpret_cast<uintptr_t>(memmap.buffer);

  uintptr_t available_end = 0;

  for (uintptr_t iter = memory_map_base;
       iter < memory_map_base + memmap.map_size;
       iter += memmap.descriptor_size) {
    auto desc = reinterpret_cast<const MemoryDescriptor *>(iter);
    if (available_end < desc->physical_start) {
      memory_manager->MarkAllocated(
          FrameID{available_end / kBytesPerFrame},
          (desc->physical_start - available_end) / kBytesPerFrame);
    }
    const auto physical_end =
        desc->physical_start + desc->number_of_pages * kUEFIPageSize;

    if (IsAvailable(static_cast<MemoryType>(desc->type))) {
      available_end = physical_end;
    } else {
      memory_manager->MarkAllocated(
          FrameID{desc->physical_start / kBytesPerFrame},
          desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
    }
  }

  memory_manager->SetMemoryRange(FrameID{1},
                                 FrameID{available_end / kBytesPerFrame});

  printk("memory available_end : %p, range_end id: %u\n", available_end,
         available_end / kBytesPerFrame);

  if (auto err = InitializeHeap(*memory_manager)) {
    Log(kError, "failed to allocate pages: %s at %s:%d\n", err.Name(),
        err.File(), err.Line());
    exit(1);
  }

#pragma endregion

#pragma region xHCの初期化

  auto err = pci::ScanAllBus();
  Log(kDebug, "ScanAllBus: %s\n", err.Name());

  pci::Device *xhcDev = nullptr;
  for (int i = 0; i < pci::num_devices; ++i) {
    auto &dev = pci::devices[i];
    auto vendorId = pci::ReadVendorId(dev.bus, dev.device, dev.function);
    auto classCode = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    Log(kDebug, "%d.%d.%d: vendor %04x, class %02x%02x%02x, head %02x\n",
        dev.bus, dev.device, dev.function, vendorId, classCode.base,
        classCode.sub, classCode.interface, dev.header_type);

    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
      xhcDev = &pci::devices[i];

      if (pci::ReadVendorId(*xhcDev) == 0x8086) {
        break;
      }
    }
  }

  if (xhcDev) {
    Log(kInfo, "xHC has been found: %d.%d.%d\n", xhcDev->bus, xhcDev->device,
        xhcDev->function);
  }

#pragma endregion

#pragma region メッセージキューの初期化

  std::array<Message, 32> main_queue_data;
  ArrayQueue<Message> main_queue{main_queue_data};
  ::main_queue = &main_queue;

#pragma endregion

#pragma region マウス割り込みの設定と起動
  const uint16_t cs = GetCS();

  SetIDTEntry(idt[InterruptVector::kXHCI],
              MakeIDTAttr(InterruptDescriptorType::kInterruptGate, 0),
              reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
  LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

  const uint8_t bsp_local_apic_id =
      *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;

  pci::ConfigureMSIFixedDestination(
      *xhcDev, bsp_local_apic_id, pci::MSITriggerMode::kLevel,
      pci::MSIDeliveryMode::kFixed, InterruptVector::kXHCI, 0);

  const auto xhcBar = pci::ReadBar(*xhcDev, 0);
  Log(kInfo, "ReadBar: %s\n", xhcBar.error.Name());

  const uint64_t xhc_mmio_base = xhcBar.value & ~static_cast<uint64_t>(0xf);
  Log(kInfo, "xHC mmio_base = 0x%08lx\n", xhc_mmio_base);

  usb::xhci::Controller xhc{xhc_mmio_base};

  if (pci::ReadVendorId(*xhcDev) == 0x8086) {
    SwitchEhci2Xhci(*xhcDev);
  }
  {
    auto err = xhc.Initialize();
    Log(kInfo, "xhc.Initialize: %s\n", err.Name());
  }

  Log(kInfo, "xHC starting\n");
  xhc.Run();

  ::xhc = &xhc;

  __asm__("sti");

  usb::HIDMouseDriver::default_observer = MouseObserver;
#pragma endregion

  for (int i = 1; i <= xhc.MaxPorts(); ++i) {
    auto port = xhc.PortAt(i);
    Log(kInfo, "Port %d: IsConnected=%d\n", i, port.IsConnected());
    if (port.IsConnected()) {
      if (auto err = usb::xhci::ConfigurePort(xhc, port)) {
        Log(kError, "failed to configure port: %s at %s:%d\n", err.Name(),
            err.File(), err.Line());
        continue;
      }
    }
  }

#pragma region レイヤー、ウインドウの初期化

  auto bgWindow = std::make_shared<Window>(screen_size.x, screen_size.y,
                                           config.pixel_format);
  DrawDesktop(*bgWindow);
  auto mouse_window = std::make_shared<Window>(
      kMouseCursorWidth, kMouseCursorHeight, config.pixel_format);

  mouse_position = {200, 200};
  mouse_window->SetTrasparentColor(kMouseTransparentColor);
  DrawMouseCursor(*mouse_window, {0, 0});

  auto main_window = std::make_shared<Window>(160, 52, config.pixel_format);
  DrawWindow(*main_window, "MainWindow");

  auto console_window = std::make_shared<Window>(
      Console::kColumns * 8, Console::kRows * 16, config.pixel_format);
  console->SetWindow(console_window);

  FrameBuffer frame_buffer;
  if (auto err = frame_buffer.Initialize(config)) {
    Log(kError,
        CreateErrorMessage("failed to initialize frame_buffer", err).c_str());
    exit(1);
  }

  layer_manager = new LayerManager;
  layer_manager->SetFrameBuffer(&frame_buffer);

  auto bglayer_id =
      layer_manager->NewLayer().SetWindow(bgWindow).Move({0, 0}).ID();
  mouse_layer_id = layer_manager->NewLayer()
                       .SetWindow(mouse_window)
                       .Move(mouse_position)
                       .ID();
  auto main_window_layer_id = layer_manager->NewLayer()
                                  .SetWindow(main_window)
                                  .SetDraggable(true)
                                  .Move({300, 300})
                                  .ID();
  auto console_layer_id =
      layer_manager->NewLayer().SetWindow(console_window).Move({0, 0}).ID();

  console->SetLayerID(console_layer_id);

  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(console_layer_id, 1);
  layer_manager->UpDown(main_window_layer_id, 2);
  layer_manager->UpDown(mouse_layer_id, 3);
  layer_manager->Draw({{0, 0}, screen_size});

#pragma endregion

  char str[128];
  unsigned int count = 0;

#pragma region メッセージループ

  for (;;) {
    ++count;
    sprintf(str, "%010u", count);
    FillRect(*main_window, {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
    WriteString(*main_window, 24, 28, {0, 0, 0}, str);
    layer_manager->Draw(main_window_layer_id);

    __asm__("cli");
    if (main_queue.Count() == 0) {
      __asm__("sti\n\thlt");
      continue;
    }

    Message msg = main_queue.Front();
    main_queue.Pop();
    __asm__("sti");

    switch (msg.type) {
      case Message::kInterruptXHCI:
        while (xhc.PrimaryEventRing()->HasFront()) {
          if (auto err = usb::xhci::ProcessEvent(xhc)) {
            Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(),
                err.File(), err.Line());
          }
        }
        break;
      default:
        Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }

#pragma endregion

  while (1) __asm__("hlt");
}

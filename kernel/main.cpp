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

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
  layer_manager->MoveRelative(mouse_layer_id, {displacement_x, displacement_y});
  StartLAPICTimer();
  layer_manager->Draw();
  auto elapsed = LAPICTimerElapased();
  StopLAPICTimer();
  printk("MouseObserver: elapsed = %u\n", elapsed);
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

alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig &config_ref,
                                   const MemoryMap &memmap_ref) {
  const FrameBufferConfig config{config_ref};
  const MemoryMap memmap{memmap_ref};

  const int kFrameWidth = config.horizontal_resolution;
  const int kFrameHeight = config.vertical_resolution;

  auto writer = CreateFrameWriter(&config);

  DrawDesktop(*pixel_writer);

  console = new (console_buf) Console(writer, {200, 200, 200}, kDesktopBGColor);

  console->SetWriter(pixel_writer);

  printk("Welcome to fuckOS!\n");
  printk("Display info: %dx%d\n", config.horizontal_resolution,
         config.vertical_resolution);

  InitializeAPICTimer();

  SetupSegments();

  const uint16_t kernel_cs = 1 << 3;
  const uint16_t kernel_ss = 2 << 3;

  SetDSAll(0);
  SetCSSS(kernel_cs, kernel_ss);

  SetupIdentityPageTable();

  ::memory_manager = new (memory_manager_buf) BitmapMemoryManager;
  const auto memory_map_base = reinterpret_cast<uintptr_t>(memmap.buffer);

  uintptr_t available_end = 0;
  uintptr_t physical_start = std::numeric_limits<uintptr_t>::max();

  for (uintptr_t iter = memory_map_base;
       iter < memory_map_base + memmap.map_size;
       iter += memmap.descriptor_size) {
    auto desc = reinterpret_cast<const MemoryDescriptor *>(iter);
    if (desc->physical_start < physical_start) {
      physical_start = desc->physical_start;
    }
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

  auto err = pci::ScanAllBus();
  printk("ScanAllBus: %s\n", err.Name());

  pci::Device *xhcDev = nullptr;
  for (int i = 0; i < pci::num_devices; ++i) {
    auto &dev = pci::devices[i];
    auto vendorId = pci::ReadVendorId(dev.bus, dev.device, dev.function);
    auto classCode = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    printk("%d.%d.%d: vendor %04x, class %02x%02x%02x, head %02x\n", dev.bus,
           dev.device, dev.function, vendorId, classCode.base, classCode.sub,
           classCode.interface, dev.header_type);

    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
      xhcDev = &pci::devices[i];

      if (pci::ReadVendorId(*xhcDev) == 0x8086) {
        break;
      }
    }
  }

  SetLogLevel(kInfo);

  if (xhcDev) {
    Log(kInfo, "xHC has been found: %d.%d.%d\n", xhcDev->bus, xhcDev->device,
        xhcDev->function);
  }

  std::array<Message, 32> main_queue_data;
  ArrayQueue<Message> main_queue{main_queue_data};
  ::main_queue = &main_queue;

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

  auto bgWindow =
      std::make_shared<Window>(kFrameWidth, kFrameHeight, config.pixel_format);
  auto bgWriter = bgWindow.get();

  DrawDesktop(*bgWriter);
  console->SetWriter(bgWriter);

  auto mouse_window = std::make_shared<Window>(
      kMouseCursorWidth, kMouseCursorHeight, config.pixel_format);

  mouse_window->SetTrasparentColor(kMouseTransparentColor);
  DrawMouseCursor(mouse_window.get(), {0, 0});

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

  mouse_layer_id =
      layer_manager->NewLayer().SetWindow(mouse_window).Move({200, 200}).ID();

  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(mouse_layer_id, 1);
  layer_manager->Draw();

  for (;;) {
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

  while (1) __asm__("hlt");
}

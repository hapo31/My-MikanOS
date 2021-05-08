#define _LIBCPP_HAS_NO_THREADS
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "console.hpp"
#include "font.hpp"
#include "frame_buffer.hpp"
#include "graphics.hpp"
#include "logger.hpp"
#include "mouse.hpp"
#include "pci.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"

const PixelColor kDesktopBGColor{100, 128, 50};
const PixelColor kDesktopFGColor{0, 255, 255};

void operator delete(void *obj) noexcept {}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

char console_buf[sizeof(Console)];
Console *console;

PixelWriter *CreatePixelContext(const struct FrameBufferConfig *config) {
  switch (config->pixel_format) {
    case kPixelRGBResv8BitPerColor:
      return pixel_writer =
                 new (pixel_writer_buf) RGBResv8BitPerColorPixelWriter;
      break;

    case kPixelBGRResv8BitPerColor:
      return pixel_writer =
                 new (pixel_writer_buf) BGRResv8BitPerColorPixelWriter;
      break;
  }
}

int WritePixel(PixelWriter *writer, const FrameBufferConfig *config, int x,
               int y, const PixelColor &c) {
  const int pixel_position = config->pixels_per_scan_line * y + x;
  uint8_t *p = &config->frame_buffer[4 * pixel_position];
  writer->Write(&p, c);
  return 0;
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

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
  mouse_cursor->MoveRelative({displacement_x, displacement_y});
  Log(kInfo, "Mouse displacement(%d, %d)\n", displacement_x, displacement_y);
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

extern "C" void KernelMain(const struct FrameBufferConfig *config) {
  uint8_t *frame_buffer = reinterpret_cast<uint8_t *>(config->frame_buffer);

  const int kFrameWidth = config->horizontal_resolution;
  const int kFrameHeight = config->vertical_resolution;

  auto writer = CreatePixelContext(config);

  FillRect(writer, config, {0, 0}, {kFrameWidth, kFrameHeight},
           kDesktopBGColor);
  // FillRect(writer,config,
  //               {0, kFrameHeight - 50},
  //               {kFrameWidth, 50},
  //               {1, 8, 17});
  // FillRect(writer,config,
  //               {0, kFrameHeight - 50},
  //               {kFrameWidth / 5, 50},
  //               {80, 80, 80});
  // DrawRect(writer,config,
  //               {10, kFrameHeight - 40},
  //               {30, 30},
  //               {160, 160, 160});

  console = new (console_buf)
      Console(writer, config, {200, 200, 200}, kDesktopBGColor);

  printk("Welcome to fuckOS!\n");
  printk("Display info: %dx%d\n", config->horizontal_resolution,
         config->vertical_resolution);

  mouse_cursor = new (mouse_cursor_buf)
      MouseCursor(writer, config, kDesktopBGColor, {300, 200});

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

  SetLogLevel(kDebug);

  if (xhcDev) {
    Log(kInfo, "xHC has been found: %d.%d.%d\n", xhcDev->bus, xhcDev->device,
        xhcDev->function);
  }

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

  while (1) {
    if (auto err = usb::xhci::ProcessEvent(xhc)) {
      Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(),
          err.File(), err.Line());
    }
  }

  while (1) __asm__("hlt");
}

#include "mouse.hpp"

#include <limits>
#include <memory>

#include "graphics.hpp"
#include "layer.hpp"
#include "logger.hpp"
#include "usb/classdriver/mouse.hpp"
#include "window.hpp"

namespace {
const char mouse_cousor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",  //
    "@@             ",  //
    "@.@            ",  //
    "@..@           ",  //
    "@...@          ",  //
    "@....@         ",  //
    "@.....@        ",  //
    "@......@       ",  //
    "@.......@      ",  //
    "@........@     ",  //
    "@.........@    ",  //
    "@..........@   ",  //
    "@...........@  ",  //
    "@............@ ",  //
    "@......@@@@@@@@",  //
    "@......@       ",  //
    "@....@@.@      ",  //
    "@...@ @.@      ",  //
    "@..@   @.@     ",  //
    "@.@    @.@     ",  //
    "@@      @.@    ",  //
    "@       @.@    ",  //
    "         @.@   ",  //
    "         @@@   ",  //
};

std::shared_ptr<Mouse> mouse;
}  // namespace

void Mouse::SetPosition(Vector2D<int> position) {
  this->position = position;
  layer_manager->Move(layer_id, position);
}

void Mouse::OnInterrupt(uint8_t buttons, int8_t displacement_x,
                        int displacement_y) {
  const auto old_pos = position;
  auto new_pos = position + Vector2D<int>{displacement_x, displacement_y};
  new_pos = ElementMin(new_pos, ScreenSize() + Vector2D<int>{-1, -1});
  position = ElementMax(new_pos, {0, 0});

  const auto pos_diff = position - old_pos;

  layer_manager->Move(layer_id, position);

  Log(kDebug, "MouseObserver: (%d,%d)\n", displacement_x, displacement_y);

  const bool previous_left_pressed = previous_buttons & 0x01;
  const bool left_pressed = buttons & 0x01;

  if (!previous_left_pressed && left_pressed) {
    auto layer = layer_manager->FindLayerByPosition(position, layer_id);
    if (layer && layer->IsDraggable()) {
      drag_layer_id = layer->ID();
      active_layer->Activate(layer->ID());
    } else {
      active_layer->Activate(0);
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

void DrawMouseCursor(PixelWriter& writer, Vector2D<int> position) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cousor_shape[dy][dx] == '@') {
        writer.Write({0, 0, 0}, position + Vector2D<int>{dx, dy});
      } else if (mouse_cousor_shape[dy][dx] == '.') {
        writer.Write({255, 255, 255}, position + Vector2D<int>{dx, dy});
      } else {
        writer.Write(kMouseTransparentColor, position + Vector2D<int>{dx, dy});
      }
    }
  }
}

void InitializeMouse() {
  auto mouse_window = std::make_shared<Window>(
      kMouseCursorWidth, kMouseCursorHeight, screen_config.pixel_format);

  mouse_window->SetTrasparentColor(kMouseTransparentColor);
  DrawMouseCursor(*mouse_window, {0, 0});
  auto mouse_layer_id = layer_manager->NewLayer().SetWindow(mouse_window).ID();
  auto mouse = std::make_shared<Mouse>(mouse_layer_id);

  mouse->SetPosition({200, 200});
  layer_manager->UpDown(mouse->LayerID(), std::numeric_limits<int>::max());

  usb::HIDMouseDriver::default_observer =
      [mouse](uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
        mouse->OnInterrupt(buttons, displacement_x, displacement_y);
      };

  active_layer->SetMouseLayer(mouse_layer_id);
}

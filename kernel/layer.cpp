#include "layer.hpp"

#include <algorithm>

#include "console.hpp"
#include "error.hpp"
#include "logger.hpp"

Layer::Layer(unsigned int id_) : id(id_) {}

Layer& Layer::Move(Vector2D<int> pos) {
  this->pos = pos;
  return *this;
}

Layer& Layer::MoveRelative(Vector2D<int> pos_diff) {
  pos += pos_diff;
  return *this;
}

void Layer::DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const {
  if (window) {
    window->DrawTo(screen, pos, area);
  }
}

void LayerManager::SetFrameBuffer(FrameBuffer* screen) {
  this->screen = screen;
  FrameBufferConfig back_config = screen->Config();
  back_config.frame_buffer = nullptr;
  if (auto err = back_buffer.Initialize(back_config)) {
    Log(kError, "back_buffer initialize failed: %s:%s in %d", err.Name(),
        err.File(), err.Line());
  }
}

void LayerManager::Draw(const Rectangle<int>& area) const {
  for (auto layer : layer_stack) {
    layer->DrawTo(*screen, area);
  }
  screen->Copy(area.pos, back_buffer, area);
}

void LayerManager::Draw(unsigned int id) const {
  bool draw = false;
  Rectangle<int> window_area;

  for (auto layer : layer_stack) {
    if (layer->ID() == id) {
      window_area.size = layer->GetWindow()->Size();
      window_area.pos = layer->GetPosition();
      draw = true;
    }

    if (draw) {
      layer->DrawTo(back_buffer, window_area);
    }
  }

  screen->Copy(window_area.pos, back_buffer, window_area);
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_pos) {
  auto layer = FindLayer(id);
  const auto window_size = layer->GetWindow()->Size();
  const auto old_pos = layer->GetPosition();
  layer->Move(new_pos);
  Draw({old_pos, window_size});
  Draw(id);
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff) {
  FindLayer(id)->MoveRelative(pos_diff);
}

void LayerManager::UpDown(unsigned int id, int new_height) {
  if (new_height < 0) {
    Hide(id);
    return;
  }

  if (new_height > layer_stack.size()) {
    new_height = layer_stack.size();
  }

  auto layer = FindLayer(id);
  auto old_pos = std::find(layer_stack.begin(), layer_stack.end(), layer);
  auto new_pos = layer_stack.begin() + new_height;

  if (old_pos == layer_stack.end()) {
    layer_stack.insert(new_pos, layer);
    return;
  }

  if (new_pos == layer_stack.end()) {
    --new_pos;
  }

  layer_stack.erase(old_pos);
  layer_stack.insert(new_pos, layer);
}

void LayerManager::Hide(unsigned int id) {
  auto layer = FindLayer(id);
  auto pos = std::find(layer_stack.begin(), layer_stack.end(), layer);
  if (pos != layer_stack.end()) {
    layer_stack.erase(pos);
  }
}

Layer* LayerManager::FindLayer(unsigned int id) {
  auto it = std::find_if(
      layers.begin(), layers.end(),
      [id](std::unique_ptr<Layer>& elem) { return elem->ID() == id; });

  if (it == layers.end()) {
    return nullptr;
  }

  return it->get();
}

Layer* LayerManager::FindLayerByPosition(Vector2D<int> pos,
                                         uint exclude_id) const {
  auto it = std::find_if(layer_stack.rbegin(), layer_stack.rend(),
                         [pos, exclude_id](Layer* layer) {
                           if (layer->ID() == exclude_id) {
                             return false;
                           }
                           const auto& win = layer->GetWindow();
                           if (!win) {
                             return false;
                           }
                           const auto win_pos = layer->GetPosition();
                           const auto win_end_pos = win_pos + win->Size();
                           return win_pos.x <= pos.x && pos.x < win_end_pos.x &&
                                  win_pos.y <= pos.y && pos.y < win_end_pos.y;
                         });

  if (it == layer_stack.rend()) {
    return nullptr;
  }
  return *it;
}

namespace {
FrameBuffer* screen;
}

LayerManager* layer_manager;

void InitializeLayer() {
  const auto screen_size = ScreenSize();
  auto bgWindow = std::make_shared<Window>(screen_size.x, screen_size.y,
                                           screen_config.pixel_format);
  DrawDesktop(*bgWindow);

  auto console_window = std::make_shared<Window>(
      Console::kColumns * 8, Console::kRows * 16, screen_config.pixel_format);

  console->SetWindow(console_window);

  screen = new FrameBuffer();
  if (auto err = screen->Initialize(screen_config)) {
    Log(kError,
        CreateErrorMessage("failed to initialize frame_buffer", err).c_str());
    exit(1);
  }

  layer_manager = new LayerManager;
  layer_manager->SetFrameBuffer(screen);

  auto bglayer_id =
      layer_manager->NewLayer().SetWindow(bgWindow).Move({0, 0}).ID();
  auto console_layer_id =
      layer_manager->NewLayer().SetWindow(console_window).Move({0, 0}).ID();

  console->SetLayerID(console_layer_id);

  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(console_layer_id, 1);
}

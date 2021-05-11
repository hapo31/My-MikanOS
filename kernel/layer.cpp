#include "layer.hpp"

#include <algorithm>

Layer::Layer(unsigned int id_) : id(id_) {}

Layer& Layer::MoveRelative(Vector2D<int> pos_diff) {
  pos += pos_diff;
  return *this;
}

void Layer::DrawTo(PixelWriter& writer) const {
  if (window) {
    window->DrawTo(writer, pos);
  }
}

Layer& Layer::Move(Vector2D<int> pos) {
  this->pos = pos;
  return *this;
}

void LayerManager::Draw() const {
  for (auto layer : layer_stack) {
    layer->DrawTo(*writer);
  }
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_position) {
  FindLayer(id)->Move(new_position);
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

LayerManager* layer_manager;

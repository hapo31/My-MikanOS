#pragma once

#include <map>
#include <memory>
#include <vector>

#include "font.hpp"
#include "graphics.hpp"
#include "message.hpp"
#include "window.hpp"

class Layer {
 public:
  Layer(unsigned int id = 0);

  unsigned int ID() const { return id; }

  Layer& SetWindow(const std::shared_ptr<Window>& window) {
    this->window = window;
    return *this;
  }
  std::shared_ptr<Window> GetWindow() const { return window; }

  Layer& Move(Vector2D<int> pos);
  Layer& MoveRelative(Vector2D<int> pos_diff);
  Layer& SetDraggable(bool draggable_) {
    draggable = draggable_;
    return *this;
  }
  bool IsDraggable() const { return draggable; }
  void DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const;

  Vector2D<int> GetPosition() const { return pos; }

 private:
  unsigned int id;
  Vector2D<int> pos;
  std::shared_ptr<Window> window;
  bool draggable = false;
};

class LayerManager {
 public:
  void SetFrameBuffer(FrameBuffer* screen);

  Layer& NewLayer() {
    ++latest_id;
    return *layers.emplace_back(new Layer{latest_id});
  }

  void Draw(const Rectangle<int>& area) const;
  void Draw(unsigned int id) const;
  void Draw(unsigned int id, Rectangle<int> area) const;

  void Move(unsigned int id, Vector2D<int> new_position);
  void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

  void UpDown(unsigned int id, int new_height);

  void Hide(unsigned int id);
  Layer* FindLayerByPosition(Vector2D<int> pos, uint exclude_id) const;
  Layer* FindLayer(unsigned int id);

  int GetHeight(unsigned int id);

  auto layer_stack_begin() const { return layer_stack.cbegin(); }
  auto layer_stack_end() const { return layer_stack.cend(); }

  auto layers_begin() const { return layers.cbegin(); }
  auto layers_end() const { return layers.cend(); }

  auto size() const { return layer_stack.size(); }

 private:
  FrameBuffer* screen{nullptr};
  mutable FrameBuffer back_buffer{};

  std::vector<std::unique_ptr<Layer>> layers{};
  std::vector<Layer*> layer_stack{};
  unsigned int latest_id{0};
};

class ActiveLayer {
 public:
  ActiveLayer(LayerManager& manager);
  void SetMouseLayer(unsigned int mouse_layer);
  void Activate(unsigned int layer_id);
  unsigned int GetActive() const { return active_layer; }

 private:
  LayerManager& manager;
  unsigned int active_layer{0};
  unsigned int mouse_layer{0};
};

extern ActiveLayer* active_layer;
extern std::map<unsigned int, uint64_t>* layer_task_map;

extern LayerManager* layer_manager;

void InitializeLayer();
void ProcessLayerMessage(const Message& msg);

constexpr Message MakeLayerMessage(uint64_t task_id, unsigned int layer_id,
                                   LayerOperation op,
                                   const Rectangle<int>& area = {{}, {}}) {
  Message msg{Message::kLayer, task_id};
  msg.arg.layer.layer_id = layer_id;
  msg.arg.layer.op = op;
  msg.arg.layer.x = area.pos.x;
  msg.arg.layer.y = area.pos.y;
  msg.arg.layer.w = area.size.x;
  msg.arg.layer.h = area.size.y;
  return msg;
}

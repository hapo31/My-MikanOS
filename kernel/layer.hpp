#pragma once

#include <map>
#include <memory>
#include <vector>

#include "graphics.hpp"
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
  void DrawTo(FrameBuffer& screen) const;

 private:
  unsigned int id;
  Vector2D<int> pos;
  std::shared_ptr<Window> window;
};

class LayerManager {
 public:
  void SetFrameBuffer(FrameBuffer* screen) { this->screen = screen; }

  Layer& NewLayer() {
    ++latest_id;
    return *layers.emplace_back(new Layer{latest_id});
  }

  void Draw() const;

  void Move(unsigned int id, Vector2D<int> new_position);
  void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

  void UpDown(unsigned int id, int new_height);

  void Hide(unsigned int id);

 private:
  FrameBuffer* screen{nullptr};

  std::vector<std::unique_ptr<Layer>> layers{};
  std::vector<Layer*> layer_stack{};
  unsigned int latest_id{0};

  Layer* FindLayer(unsigned int id);
};

extern LayerManager* layer_manager;

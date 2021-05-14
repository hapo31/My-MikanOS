#pragma once

#include <optional>
#include <vector>

#include "frame_buffer.hpp"
#include "graphics.hpp"

class Window : public PixelWriter {
 public:
  Window(int width, int height, PixelFormat shadow_format);
  ~Window() = default;

  Window(const Window& rhs) = delete;
  Window& operator=(const Window& rhs) = delete;

  void Write(const PixelColor& c, int x, int y) override;
  void Move(Vector2D<int> dest_pos, const Rectangle<int>& src);

  int Width() const override { return width; }
  int Height() const override { return height; }

  Vector2D<int> Size() const { return {width, height}; }

  void DrawTo(FrameBuffer& dest, Vector2D<int> position,
              const Rectangle<int>& area);
  void SetTrasparentColor(std::optional<PixelColor> c) {
    transparent_color = c;
  }

  PixelColor& At(int x, int y) { return data[y][x]; }
  const PixelColor& At(int x, int y) const { return data[y][x]; }

 private:
  int width, height;

  std::vector<std::vector<PixelColor>> data{};
  std::optional<PixelColor> transparent_color{std::nullopt};

  FrameBuffer shadow_buffer{};
};

void DrawWindow(PixelWriter& writer, const char* title);

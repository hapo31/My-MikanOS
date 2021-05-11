#pragma once

#include <optional>
#include <vector>

#include "graphics.hpp"

class Window {
 public:
  class WindowWriter : public PixelWriter {
   public:
    WindowWriter(Window& window_) : window(window_) {}
    virtual void Write(const PixelColor& color, int x, int y) override {
      window.At(x, y) = color;
    }

    virtual int Width() const override { return window.Width(); }
    virtual int Height() const override { return window.Height(); }

   private:
    Window& window;
  };

  Window(int width, int height);
  ~Window() = default;

  Window(const Window& rhs) = delete;
  Window& operator=(const Window& rhs) = delete;

  void DrawTo(PixelWriter& writer, Vector2D<int> position);
  void SetTrasparentColor(std::optional<PixelColor> c) {
    transparent_color = c;
  }

  WindowWriter* Writer() { return &writer; }
  PixelColor& At(int x, int y) { return data[y][x]; }
  const PixelColor& At(int x, int y) const { return data[y][x]; }

  int Width() const { return width; }
  int Height() const { return height; }

 private:
  int width, height;

  std::vector<std::vector<PixelColor>> data{};
  WindowWriter writer{*this};
  std::optional<PixelColor> transparent_color{std::nullopt};
};

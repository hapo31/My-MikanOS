#pragma once

#include <optional>
#include <vector>

#include "frame_buffer.hpp"
#include "graphics.hpp"
#include "logger.hpp"

class Window : public PixelWriter {
 public:
  Window(int width, int height, PixelFormat shadow_format);
  virtual ~Window() = default;

  Window(const Window& rhs) = delete;
  Window& operator=(const Window& rhs) = delete;

  void Write(const PixelColor& c, int x, int y) override;
  virtual void Move(Vector2D<int> dest_pos, const Rectangle<int>& src);

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

  virtual void Activate() {}
  virtual void Deactivate() {}

 private:
  int width, height;

  std::vector<std::vector<PixelColor>> data{};
  std::optional<PixelColor> transparent_color{std::nullopt};
  FrameBuffer shadow_buffer{};
};

class ToplevelWindow : public Window {
 public:
  static constexpr Vector2D<int> kTopLeftMargin{4, 24};
  static constexpr Vector2D<int> kBottomRightMargin{4, 4};
  static constexpr int kMarginX = kTopLeftMargin.x + kBottomRightMargin.x;
  static constexpr int kMarginY = kTopLeftMargin.y + kBottomRightMargin.y;

  class WindowInnerWriter : public PixelWriter {
   public:
    WindowInnerWriter(ToplevelWindow& window_) : window(window_) {}

    void Write(const PixelColor& c, int x, int y) override {
      window.Write(c, x + kTopLeftMargin.x, y + kTopLeftMargin.y);
    }

    int Width() const override { return window.Width() - kMarginX; }
    int Height() const override { return window.Height() - kMarginY; }

   private:
    ToplevelWindow& window;
  };

  ToplevelWindow(int width, int height, PixelFormat shadow_format,
                 const std::string& title);

  virtual void Move(Vector2D<int> dest_pos, const Rectangle<int>& src) override;
  virtual void Activate() override;
  virtual void Deactivate() override;

  Vector2D<int> InnerSize() const;
  WindowInnerWriter& InnerWriter() { return inner_writer; }

 private:
  WindowInnerWriter inner_writer{*this};
  std::string title;
};

void DrawWindow(PixelWriter& writer, const char* title);
void DrawTextBox(PixelWriter& writer, Vector2D<int> pos, Vector2D<int> size);
void DrawTerminal(PixelWriter& writer, Vector2D<int> pos, Vector2D<int> size);
void DrawWindowTitle(PixelWriter& writer, const char* title, bool active);

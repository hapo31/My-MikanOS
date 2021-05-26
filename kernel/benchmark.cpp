#include "benchmark.hpp"

#include "layer.hpp"
#include "task.hpp"
#include "window.hpp"

namespace {
unsigned int counter;
std::shared_ptr<ToplevelWindow> benchmark_window;
unsigned int layer_id;

const int win_w = 100;
const int win_h = 40;

}  // namespace

void InitializeBenchMark() {
  benchmark_window = std::make_shared<ToplevelWindow>(
      win_w, win_h, screen_config.pixel_format, "geekbench");

  FillRect(benchmark_window->InnerWriter(), {0, 0}, {win_w - 1, win_h - 1},
           ToColor(0x000000));

  layer_id = layer_manager->NewLayer()
                 .SetWindow(benchmark_window)
                 .SetDraggable(true)
                 .Move({200, 100})
                 .ID();
}

void TaskBenchMark(uint64_t task_id, int64_t data) {
  asm("cli");
  Task& task = task_manager->CurrentTask();
  active_layer->Activate(layer_id);
  asm("sti");

  while (true) {
    asm("cli");
    auto msg = task.ReceiveMessage();
    if (!msg) {
      task.Sleep();
      asm("sti");
      continue;
    }

    switch (msg->type) {
      case Message::kTimerTimeout: {
        counter = task_manager->Counter();
        char buf[16];
        sprintf(buf, "%u", counter);
        FillRect(benchmark_window->InnerWriter(), {0, 0},
                 {win_w - 1, win_h - 1}, ToColor(0x000000));
        WriteString(benchmark_window->InnerWriter(), 0, 0, ToColor(0xffffff),
                    buf);
        task_manager->SetCounter(0);
        auto msg = MakeLayerMessage(task_id, layer_id, LayerOperation::Draw);
        asm("cli");
        task_manager->SendMessage(1, msg);
        asm("sti");
      } break;
      default:
        break;
    }
  }
}

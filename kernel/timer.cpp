#include "timer.hpp"

#include "interrupt.hpp"
#include "logger.hpp"
#include "message.hpp"

namespace {

const uint32_t kCountMax = 0xffffffffu;
volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(0xfee00320);
volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(0xfee00380);
volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(0xfee00390);
volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(0xfee003e0);

}  // namespace

TimerManager::TimerManager(std::deque<Message>& msg_queue_)
    : msg_queue{msg_queue_} {
  timers.emplace(Timer{std::numeric_limits<unsigned long>::max(), -1});
}

void TimerManager::Tick() {
  ++tick;

  while (true) {
    const auto& t = timers.top();
    if (t.Timeout() > tick) {
      break;
    }
    Message m{Message::kTimerTimeout};
    m.arg.timer.timeout = t.Timeout();
    m.arg.timer.value = t.Value();

    msg_queue.emplace_back(m);

    timers.pop();
  }
}

void TimerManager::AddTimer(const Timer& timer) { timers.emplace(timer); }

TimerManager* timer_manager;

void InitializeLAPICTimer(std::deque<Message>& msg_queue) {
  timer_manager = new TimerManager(msg_queue);
  divide_config = 0b1011;
  lvt_timer = (0b010 << 16) | InterruptVector::kLAPICTimer;
  initial_count = 0x1000000u;
}

void LAPICTimerOnIterrupt() { timer_manager->Tick(); }

void StartLAPICTimer() { initial_count = kCountMax; }

uint32_t LAPICTimerElapased() { return kCountMax - current_count; }

void StopLAPICTimer() { initial_count = 0; }

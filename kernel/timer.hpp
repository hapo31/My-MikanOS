#pragma once
#include <cstdint>
#include <deque>
#include <queue>

#include "message.hpp"

class Timer {
 public:
  Timer(unsigned long timeout_, int value_)
      : timeout(timeout_), value(value_) {}
  unsigned long Timeout() const { return timeout; }
  int Value() const { return value; }

 private:
  unsigned long timeout;
  int value;
};

class TimerManager {
 public:
  TimerManager();
  void AddTimer(const Timer& timer);
  bool Tick();
  unsigned long CurrentTick() const { return tick; }

 private:
  volatile unsigned long tick{0};
  std::priority_queue<Timer> timers{};
};

inline bool operator<(const Timer& lhs, const Timer& rhs) {
  return lhs.Timeout() > rhs.Timeout();
}

extern TimerManager* timer_manager;
extern unsigned long lapic_timer_freq;
const int kTimerFreq = 100;

const int kTaskTimerPeriod = static_cast<int>(kTimerFreq * 0.02);
const int kTaskTimerValue = std::numeric_limits<int>::min();

void InitializeLAPICTimer();
void LAPICTimerOnInterrupt();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

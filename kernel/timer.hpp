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
  TimerManager(std::deque<Message>& msg_queue);
  void AddTimer(const Timer& timer);
  void Tick();
  unsigned long CurrentTick() const { return tick; }

 private:
  volatile unsigned long tick{0};
  std::priority_queue<Timer> timers{};
  std::deque<Message>& msg_queue;
};

inline bool operator<(const Timer& lhs, const Timer& rhs) {
  return lhs.Timeout() > rhs.Timeout();
}

extern TimerManager* timer_manager;

void InitializeLAPICTimer(std::deque<Message>& msg_queue);
void LAPICTimerOnIterrupt();
void StartLAPICTimer();
uint32_t LAPICTimerElapased();
void StopLAPICTimer();

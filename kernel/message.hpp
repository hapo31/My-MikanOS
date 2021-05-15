#pragma once

struct Message {
  enum Type {
    kInterruptXHCI,
    kInterruptTimer,
    kTimerTimeout,
  } type;

  union {
    struct {
      unsigned long timeout;
      int value;
    } timer;
  } arg;
};

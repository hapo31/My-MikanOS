#include "task.hpp"

#include "asmfunc.h"
#include "logger.hpp"
#include "timer.hpp"

alignas(16) TaskContext lifegame_context{.name = "life"}, main_context{
                                                              .name = "main"};

namespace {
TaskContext* current_task;
}

void SwitchTask() {
  TaskContext* old_current_task = current_task;
  if (current_task == &main_context) {
    current_task = &lifegame_context;
  } else {
    current_task = &main_context;
  }

  SwitchContext(current_task, old_current_task);
}

void InitializeTask() {
  current_task = &main_context;

  __asm__("cli");
  timer_manager->AddTimer(
      Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue});
  __asm__("sti");
}

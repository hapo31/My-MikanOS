#include "task.hpp"

#include <string.h>

#include "asmfunc.h"
#include "logger.hpp"
#include "segment.hpp"
#include "timer.hpp"

namespace {
TaskContext* current_task;
}  // namespace

Task& Task::InitContext(TaskFunc* f, int64_t data) {
  const size_t stack_size = kDefaultStackBytes / sizeof(stack[0]);
  stack.resize(stack_size);
  uint64_t stack_end = reinterpret_cast<uint64_t>(&stack[stack_size]);

  memset(&context, 0, sizeof(context));
  context.cr3 = GetCR3();
  context.rflags = 0x202;
  context.cs = kKernelCS;
  context.ss = kKernelSS;
  context.rsp = (stack_end & ~0xflu) - 8;

  context.rip = reinterpret_cast<uint64_t>(f);
  context.rdi = id;
  context.rsi = data;

  *reinterpret_cast<uint32_t*>(&context.fxsave_area[24]) = 0x1f80;

  return *this;
}

TaskManager::TaskManager() { NewTask(); }

Task& TaskManager::NewTask() {
  ++latest_id;
  return *tasks.emplace_back(new Task{latest_id});
}

void TaskManager::SwitchTask() {
  size_t next_task_index = (current_task_index + 1) % tasks.size();

  Task& current_task = *tasks[current_task_index];
  Task& next_task = *tasks[next_task_index];
  current_task_index = next_task_index;

  SwitchContext(&next_task.Context(), &current_task.Context());
}

TaskManager* task_manager;

void InitializeTask() {
  ::task_manager = new TaskManager();

  __asm__("cli");
  timer_manager->AddTimer(
      Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue});
  __asm__("sti");
}

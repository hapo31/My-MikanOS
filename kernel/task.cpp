#include "task.hpp"

#include <string.h>

#include <algorithm>

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

Task& Task::Sleep() {
  task_manager->Sleep(id);
  return *this;
}
Task& Task::Wakeup() {
  task_manager->Wakeup(id);
  return *this;
}

TaskManager::TaskManager() { running.emplace_back(&NewTask()); }

Task& TaskManager::NewTask() {
  ++latest_id;
  return *tasks.emplace_back(new Task{latest_id});
}

void TaskManager::SwitchTask(bool current_sleep) {
  auto current_task = running.front();

  running.pop_front();
  if (!current_sleep) {
    running.push_back(current_task);
  }

  Task* next_task = running.front();

  SwitchContext(&next_task->Context(), &current_task->Context());
}

void TaskManager::Sleep(Task* task) {
  auto it = std::find(running.begin(), running.end(), task);
  if (it == running.begin()) {
    SwitchTask(true);
    return;
  }

  if (it == running.end()) {
    return;
  }

  running.erase(it);
}

Error TaskManager::Sleep(uint64_t task_id) {
  auto it = std::find_if(
      tasks.begin(), tasks.end(),
      [task_id](const auto& task) { return task->ID() == task_id; });

  if (it == tasks.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Sleep(it->get());

  return MAKE_ERROR(Error::kSuccess);
}

void TaskManager::Wakeup(Task* task) {
  auto it = std::find(running.begin(), running.end(), task);

  if (it == running.end()) {
    running.emplace_back(task);
  }
}

Error TaskManager::Wakeup(uint64_t task_id) {
  auto it = std::find_if(
      tasks.begin(), tasks.end(),
      [task_id](const auto& task) { return task->ID() == task_id; });

  if (it == tasks.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Wakeup(it->get());
  return MAKE_ERROR(Error::kSuccess);
}

TaskManager* task_manager;

void InitializeTask() {
  ::task_manager = new TaskManager();

  __asm__("cli");
  timer_manager->AddTimer(
      Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue});
  __asm__("sti");
}

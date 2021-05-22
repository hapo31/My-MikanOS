#include "task.hpp"

#include <string.h>

#include <algorithm>

#include "acpi.hpp"
#include "asmfunc.h"
#include "logger.hpp"
#include "segment.hpp"
#include "timer.hpp"
#include "utils.hpp"

namespace {
TaskContext* current_task;
}  // namespace

Task::Task(uint64_t id_, level_t level_) : id(id_), level(level_) {}

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
  task_manager->Sleep(this);
  return *this;
}

Task& Task::Wakeup() {
  task_manager->Wakeup(this);
  return *this;
}

void Task::SendMessage(const Message& msg) {
  msgs.emplace_back(msg);
  Wakeup();
}

std::optional<Message> Task::ReceiveMessage() {
  if (msgs.empty()) {
    return std::nullopt;
  }
  auto m = msgs.front();
  msgs.pop_front();

  return m;
}

TaskManager::TaskManager() {
  Task& task = NewTask().SetLevel(current_level).SetRunning(true);
  running[current_level].emplace_back(&task);
}

Task& TaskManager::NewTask(level_t level) {
  ++latest_id;
  return *tasks.emplace_back(new Task{latest_id, level});
}

void TaskManager::SwitchTask(bool current_sleep) {
  auto& level_queue = running[current_level];
  Task* current_task = level_queue.front();

  level_queue.pop_front();

  if (!current_sleep) {
    level_queue.emplace_back(current_task);
  }

  if (level_queue.empty()) {
    level_changed = true;
  }

  if (level_changed) {
    level_changed = false;
    for (int lv = kLevelMax; lv >= 0; --lv) {
      if (!running[lv].empty()) {
        current_level = lv;
        break;
      }
    }
  }

  Task* next_task = running[current_level].front();
  SwitchContext(&next_task->Context(), &current_task->Context());
}

void TaskManager::Sleep(Task* task) {
  if (!task->Running()) {
    return;
  }

  task->SetRunning(false);

  if (task == running[current_level].front()) {
    SwitchTask(true);
    return;
  }

  Erase(running[task->Level()], task);
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

void TaskManager::Wakeup(Task* task, level_t level) {
  if (task->Running()) {
    ChangeLevelRunning(task, level);
    return;
  }

  if (level < 0) {
    level = task->Level();
  }

  task->SetLevel(level);
  task->SetRunning(true);

  running[level].emplace_back(task);

  if (level > current_level) {
    level_changed = true;
  }
  return;
}

Error TaskManager::Wakeup(uint64_t task_id, level_t level) {
  auto it = std::find_if(
      tasks.begin(), tasks.end(),
      [task_id](const auto& task) { return task->ID() == task_id; });

  if (it == tasks.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Wakeup(it->get(), level);
  return MAKE_ERROR(Error::kSuccess);
}

Error TaskManager::SendMessage(uint64_t id, const Message& msg) {
  auto it = std::find_if(tasks.begin(), tasks.end(),
                         [id](const auto& task) { return task->ID() == id; });

  if (it == tasks.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  (*it)->SendMessage(msg);
  return MAKE_ERROR(Error::kSuccess);
}

Task& TaskManager::CurrentTask() { return *running[current_level].front(); }

void TaskManager::ChangeLevelRunning(Task* task, level_t level) {
  if (level < 0 || level == task->Level()) {
    return;
  }

  if (task != running[current_level].front()) {
    Erase(running[task->Level()], task);
    running[level].emplace_back(task);
    task->SetLevel(level);
    if (level > current_level) {
      level_changed = true;
    }
    return;
  }
  running[current_level].pop_front();
  running[level].emplace_front(task);
  task->SetLevel(level);
  if (level >= current_level) {
    current_level = level;
  } else {
    current_level = level;
    level_changed = true;
  }
}

TaskManager* task_manager;

void InitializeTask() {
  ::task_manager = new TaskManager();

  __asm__("cli");
  timer_manager->AddTimer(
      Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue});
  __asm__("sti");
}

#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <vector>

#include "error.hpp"
#include "message.hpp"

struct TaskContext {
  uint64_t cr3, rip, rflags, reserved1;
  uint64_t cs, ss, fs, gs;
  uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rpb;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  std::array<uint8_t, 512> fxsave_area;
  const char* name;
} __attribute__((packed));

extern TaskContext lifegame_context, main_context;

using TaskFunc = void(uint64_t, uint64_t);

class Task {
 public:
  static const size_t kDefaultStackBytes = 4096;

  Task(uint64_t id_) : id(id_) {}
  Task& InitContext(TaskFunc* f, int64_t data);
  TaskContext& Context() { return context; }

  uint64_t ID() const { return id; }

  Task& Sleep();
  Task& Wakeup();

  void SendMessage(const Message& msg);
  std::optional<Message> ReceivedMessage();

 private:
  uint64_t id;
  std::vector<uint64_t> stack;
  alignas(16) TaskContext context;
  std::deque<Message> msgs;
};

class TaskManager {
 public:
  TaskManager();
  Task& NewTask();
  void SwitchTask(bool current_sleep = false);

  void Sleep(Task* task);
  Error Sleep(uint64_t task_id);

  void Wakeup(Task* task);
  Error Wakeup(uint64_t task_id);

  Error SendMessage(uint64_t id, const Message& msg);

  Task& CurrentTask();

 private:
  std::vector<std::unique_ptr<Task>> tasks;
  uint64_t latest_id{0};
  std::deque<Task*> running{};
  size_t current_task_index{0};
};

extern TaskManager* task_manager;

void SwitchTask();
void InitializeTask();

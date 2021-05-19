#pragma once
#include <array>

struct TaskContext {
  uint64_t cr3, rip, rflags, reserved1;
  uint64_t cs, ss, fs, gs;
  uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rpb;
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
  std::array<uint8_t, 512> fxsave_area;
  const char* name;
} __attribute__((packed));

extern TaskContext lifegame_context, main_context;

void SwitchTask();
void InitializeTask();

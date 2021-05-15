#pragma once

#include <array>
#include <cstdio>
#include <deque>

enum class InterruptDescriptorType {
  kUpper8Bytes = 0,
  kLDT = 2,
  kTSSAvailable = 9,
  kTSSBusy = 11,
  kCallGate = 12,
  kInterruptGate = 14,
  kTrapGate = 15,
};

struct Message {
  enum Type {
    kInterruptXHCI,
  } type;
};

union InterruptDesriptorAttribute {
  uint16_t data;
  struct {
    uint16_t interrupt_stack_table : 3;
    uint16_t : 5;
    InterruptDescriptorType type : 4;
    uint16_t : 1;
    uint16_t descritor_privilege_level : 2;
    uint16_t present : 1;
  } __attribute__((packed)) bits;
} __attribute__((packed));

struct InterruptDescriptor {
  uint16_t offset_low;
  uint16_t segment_selector;
  InterruptDesriptorAttribute attr;
  uint16_t offset_middle;
  uint32_t offset_high;
  uint32_t reserved;
} __attribute__((packed));

extern std::array<InterruptDescriptor, 256> idt;

constexpr InterruptDesriptorAttribute MakeIDTAttr(
    InterruptDescriptorType type, uint8_t descriptor_privilege_level,
    bool present = true, uint8_t interrupt_stack_table = 0) {
  InterruptDesriptorAttribute attr{};
  attr.bits.interrupt_stack_table = interrupt_stack_table;
  attr.bits.type = type;
  attr.bits.descritor_privilege_level = descriptor_privilege_level;
  attr.bits.present = present;

  return attr;
}

void SetIDTEntry(InterruptDescriptor& desc, InterruptDesriptorAttribute attr,
                 uint64_t offset, uint16_t segment_selector);

class InterruptVector {
 public:
  enum Number {
    kXHCI = 0x40,
  };
};

struct InterruptFrame {
  uint64_t rip, cs, frlags, rsp, ss;
};

void NotifyEndOfInterrupt();
void InitializeInterrupt(std::deque<Message>* msg_queue);

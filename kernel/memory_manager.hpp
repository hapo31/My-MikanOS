#pragma once

#include <array>
#include <limits>

#include "error.hpp"
#include "memory_map.hpp"

namespace {
constexpr unsigned long long operator"" _KiB(unsigned long long kib) {
  return kib * 1024;
}

constexpr unsigned long long operator"" _MiB(unsigned long long mib) {
  return mib * 1024_KiB;
}

constexpr unsigned long long operator"" _GiB(unsigned long long gib) {
  return gib * 1024_MiB;
}
}  // namespace

static const auto kBytesPerFrame{4_KiB};

class FrameID {
 public:
  explicit FrameID(size_t id_) : id(id_) {}
  size_t ID() const { return id; }
  void* Frame() const { return reinterpret_cast<void*>(id * kBytesPerFrame); }

 private:
  size_t id;
};

static const FrameID kNullFrame{std::numeric_limits<size_t>::max()};

class BitmapMemoryManager {
 public:
  static const auto kMaxPhysicalMemoryBytes{128_GiB};
  static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};

  using MapLineType = unsigned long;

  static const size_t kBitsPerMapLine{8 * sizeof(MapLineType)};

  BitmapMemoryManager();

  WithError<FrameID> Allocate(size_t num_frames);
  Error Free(FrameID start_frame, size_t num_frames);
  void MarkAllocated(FrameID start_frame, size_t num_frames);

  void SetMemoryRange(FrameID range_begin, FrameID range_end);

 private:
  static const auto kAllocMapSize = kFrameCount / kBitsPerMapLine;
  std::array<MapLineType, kAllocMapSize> alloc_map;

  FrameID range_begin;
  FrameID range_end;

  bool GetBit(FrameID frame) const;
  void SetBit(FrameID frame, bool allocated);
};

void InitializeMemoryManager(const MemoryMap& memmap);

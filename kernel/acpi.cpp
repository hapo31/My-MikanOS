#include "acpi.hpp"

#include <cstdlib>
#include <cstring>

#include "logger.hpp"

namespace {
template <typename T>
uint8_t SumBytes(const T* data, size_t bytes) {
  return SumBytes(reinterpret_cast<const uint8_t*>(data), bytes);
}

template <>
uint8_t SumBytes<uint8_t>(const uint8_t* data, size_t bytes) {
  uint8_t sum = 0;
  for (size_t i = 0; i < bytes; ++i) {
    sum += data[i];
  }
  return sum;
}

}  // namespace

namespace acpi {
bool RSDP::IsValid() const {
  if (strncmp(signature, "RSD PTR ", 8) != 0) {
    Log(kDebug, "invalid signature: %.8s\n", signature);
    return false;
  }

  if (revision != 2) {
    Log(kDebug, "ACPI revision must be 2: %d\n", revision);
    return false;
  }

  if (auto sum = SumBytes(this, 20); sum != 0) {
    Log(kDebug, "sum of 36 bytes must be 0: %d\n", sum);
    return false;
  }
  if (auto sum = SumBytes(this, 36); sum != 0) {
    Log(kDebug, "sum of 36 bytes must be 0: %d\n", sum);
    return false;
  }

  return true;
}

void Initialize(const RSDP& rsdp) {
  if (!rsdp.IsValid()) {
    Log(kError, "RSDP is not valid\n");
    exit(1);
  }
}

}  // namespace acpi
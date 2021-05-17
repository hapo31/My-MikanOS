#pragma once
#include <cstddef>
#include <cstdint>

namespace acpi {

struct FADT;

struct DescriptionHeader {
  char signature[4];
  uint32_t length;
  uint8_t revisition;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revisition;
  uint32_t creator_id;
  uint32_t creator_revisition;
  bool IsValid(const char* expected_signature) const;
  auto ToFADTAddr() const { return reinterpret_cast<const FADT*>(this); }
} __attribute__((packed));

struct XSDT {
  DescriptionHeader header;
  const DescriptionHeader& operator[](size_t i) const;
  size_t Count() const;
} __attribute__((packed));

struct RSDP {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t revision;
  uint32_t rsdt_address;
  uint32_t length;
  uint64_t xsdt_address;
  uint8_t extended_checksum;
  char reserved[3];

  bool IsValid() const;
  auto GetXSDTAddr() const {
    return reinterpret_cast<const XSDT*>(xsdt_address);
  }
} __attribute__((packed));

struct FADT {
  DescriptionHeader header;
  char reserved1[76 - sizeof(header)];
  uint32_t pm_tmr_blk;
  char reserved2[112 - 80];
  uint32_t flags;
  char reserved3[276 - 116];
} __attribute__((packed));

extern const FADT* fadt;

void Initialize(const RSDP& rsdp);
}  // namespace acpi

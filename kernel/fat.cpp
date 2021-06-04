#include "fat.hpp"

#include <cstring>

namespace fat {
BPB* boot_volume_image;

void Initialize(void* volume_image) {
  boot_volume_image = reinterpret_cast<fat::BPB*>(volume_image);
}

uintptr_t GetClusterAddr(unsigned long cluster) {
  unsigned long sector_num =
      boot_volume_image->reserved_sector_count +
      boot_volume_image->num_fats * boot_volume_image->fat_size_32 +
      (cluster - 2) * boot_volume_image->sectors_per_cluster;

  uintptr_t offset = sector_num * boot_volume_image->bytes_per_sector;
  return reinterpret_cast<uintptr_t>(boot_volume_image) + offset;
}

std::tuple<std::string, std::string> ReadName(const DirectoryEntry& entry) {
  std::string base;
  base.resize(8);
  memcpy(&base[0], &entry.name[0], 8);
  for (int i = 7; i >= 0 && base[i] == 0x20; --i) {
    base[i] = 0;
  }

  std::string ext;
  ext.resize(3);
  for (int i = 2; i >= 0 && ext[i] == 0x20; --i) {
    ext[i] = 0;
  }

  return std::make_tuple(base, ext);
}

}  // namespace fat

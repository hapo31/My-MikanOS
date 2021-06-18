#include "fat.hpp"

#include <cstring>

namespace fat {
BPB* boot_volume_image;
unsigned long bytes_per_cluster;

void Initialize(void* volume_image) {
  boot_volume_image = reinterpret_cast<fat::BPB*>(volume_image);
  bytes_per_cluster =
      static_cast<unsigned long>(boot_volume_image->bytes_per_sector) *
      boot_volume_image->sectors_per_cluster;
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

unsigned long NextCluster(unsigned long cluster) {
  uintptr_t fat_offset = boot_volume_image->reserved_sector_count *
                         boot_volume_image->bytes_per_sector;

  uint32_t* fat = reinterpret_cast<uint32_t*>(
      reinterpret_cast<uintptr_t>(boot_volume_image) + fat_offset);

  uint32_t next = fat[cluster];
  if (next >= 0x0ffffff8ul) {
    return kEndOfClusterchain;
  }
  return next;
}

DirectoryEntry* FindFile(const char* name, unsigned long directory_cluster) {
  if (directory_cluster == 0) {
    directory_cluster = boot_volume_image->root_cluster;
  }
  while (directory_cluster != kEndOfClusterchain) {
    auto dir = GetSectorByCluster<DirectoryEntry>(directory_cluster);
    for (int i = 0; i < bytes_per_cluster / sizeof(DirectoryEntry); ++i) {
      if (NameIsEqual(dir[i], name)) {
        return &dir[i];
      }
    }

    directory_cluster = NextCluster(directory_cluster);
  }

  return nullptr;
}

bool NameIsEqual(const DirectoryEntry& entry, const char* name) {
  unsigned char name83[11];
  memset(name83, 0x20, sizeof(name83));

  int i = 0;
  int i83 = 0;
  for (; name[i] != 0 && i83 < sizeof(name83); ++i, ++i83) {
    if (name[i] == '.') {
      i83 = 7;
      continue;
    }
    name83[i83] = toupper(name[i]);
  }
  return memcmp(entry.name, name83, sizeof(name83)) == 0;
}

}  // namespace fat

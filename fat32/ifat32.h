#ifndef IFAT32_H
#define IFAT32_H
#include <cstdint>
#include <string>
#include <utility>

class IFAT32 {
 public:
  virtual ~IFAT32() = default;
  virtual bool Init(const std::string& image_path) = 0;
  virtual std::pair<uint16_t, uint32_t>
  get_image_sector_size_and_total_sectors_32() = 0;
  virtual bool read(uint32_t lba, uint32_t offset, uint8_t* buffer,
                    uint32_t bufsize) = 0;
  virtual bool write(uint32_t lba, uint32_t offset, const uint8_t* buffer,
                     uint32_t bufsize) = 0;

 protected:
#pragma pack(push, 1)
  // FAT32 Boot Sector, uint8_ts 0-35
  struct BPB_FAT {
    uint8_t jump[3];              // 0x00
    char oem_name[8];             // 0x03
    uint16_t sector_size;         // 0x0B
    uint8_t sectors_per_cluster;  // 0x0D
    uint16_t reserved_sectors;    // 0x0E
    uint8_t num_fats;             // 0x10
    uint16_t root_entries;        // 0x11
    uint16_t total_sectors_16;    // 0x13
    uint8_t media_type;           // 0x15
    uint16_t fat_size_16;         // 0x16
    uint16_t sectors_per_track;   // 0x18
    uint16_t num_heads;           // 0x1A
    uint32_t hidden_sectors;      // 0x1C
    uint32_t total_sectors_32;    // 0x20
  };
#pragma pack(pop)
};

#endif  // IFAT32_H

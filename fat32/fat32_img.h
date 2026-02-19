#ifndef FAT32_H
#define FAT32_H
#include <cstdint>
#include <string>
#include <utility>

#include "ifat32.h"

class FAT32_IMG : public IFAT32 {
 public:
  FAT32_IMG() = default;
  bool Init(const std::string& image_path) override;
  std::pair<uint16_t, uint32_t> get_image_sector_size_and_total_sectors_32()
      override;
  bool read(uint32_t lba, uint32_t offset, uint8_t* buffer,
            uint32_t bufsize) override;
  bool write(uint32_t lba, uint32_t offset, const uint8_t* buffer,
             uint32_t bufsize) override;

 private:
  static bool read_from_image(const char* image_file, uint64_t pos,
                              uint8_t* buffer, uint32_t bufsize);
  static bool write_to_image(const char* image_file, uint64_t pos,
                             const uint8_t* buffer, uint32_t bufsize);
  std::string image_path_ =
      "/home/maxmyk/Downloads/wifi-filesystem/cmake-build-debug/test.img";
};

#endif  // FAT32_H

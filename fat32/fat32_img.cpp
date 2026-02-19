#include "fat32_img.h"

#include <fstream>
#include <iostream>

bool FAT32_IMG::Init(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Error: Cannot open image file: " << path << std::endl;
    return false;
  }
  image_path_ = path;
  return true;
}

std::pair<uint16_t, uint32_t>
FAT32_IMG::get_image_sector_size_and_total_sectors_32() {
  std::ifstream file(image_path_.c_str(), std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return std::make_pair(0, 0);
  }
  BPB_FAT bpb{};
  file.read((char*)&bpb, sizeof(bpb));
  return std::make_pair(bpb.sector_size, bpb.total_sectors_32);
}

bool FAT32_IMG::read(uint32_t lba, uint32_t offset, uint8_t* buffer,
                     uint32_t bufsize) {
  const uint64_t pos = lba * 512 + offset;
  return read_from_image(image_path_.c_str(), pos, buffer, bufsize);
}

bool FAT32_IMG::write(uint32_t lba, uint32_t offset, const uint8_t* buffer,
                      uint32_t bufsize) {
  const uint64_t pos = lba * 512 + offset;
  return write_to_image(image_path_.c_str(), pos, buffer, bufsize);
}

bool FAT32_IMG::read_from_image(const char* image_file, uint64_t pos,
                                uint8_t* buffer, uint32_t bufsize) {
  std::ifstream file(image_file, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return false;
  }
  file.seekg(pos);
  file.read((char*)buffer, bufsize);
  return true;
}

bool FAT32_IMG::write_to_image(const char* image_file, uint64_t pos,
                               const uint8_t* buffer, uint32_t bufsize) {
  std::ofstream file(image_file,
                     std::ios::binary | std::ios::in | std::ios::out);
  if (!file.is_open()) {
    std::cerr << "Error opening file." << std::endl;
    return false;
  }
  file.seekp(pos);
  file.write((char*)buffer, bufsize);
  return true;
}

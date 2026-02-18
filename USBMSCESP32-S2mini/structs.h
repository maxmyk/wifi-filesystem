#ifndef STRUCTS_H
#define STRUCTS_H

#include <cstdint>

constexpr int BUFFER_SIZE = 32768;

struct remote_request
{
  uint64_t position{};
  uint32_t length{};
} __attribute__((packed));

struct init_answer
{
  unsigned int sectors_count;
  unsigned int sector_size;
  unsigned int reserved[6];
};

enum BUFFER_STATUSES
{
  BUFFER_STATUS_NONE = 0,
  BUFFER_STATUS_NEED_REQUEST,
  BUFFER_STATUS_IN_REQUEST,
  BUFFER_STATUS_ERROR,
  BUFFER_STATUS_DONE
};

enum REQUEST_TYPES
{
  REQUEST_TYPE_INVALID = 0,
  REQUEST_TYPE_INIT = 1,
  REQUEST_TYPE_READ = 2,
  REQUEST_TYPE_WRITE = 3
};

struct sectors_buffer
{
  BUFFER_STATUSES status = BUFFER_STATUS_NONE;
  int64_t  lba = 0;
  uint32_t offset = 0;
  uint32_t bufsize = BUFFER_SIZE;
  uint32_t recv_offset = 0;
  uint32_t recv_len = 0;
  uint8_t  buffer[BUFFER_SIZE]{};
} __attribute__((packed));

#endif // STRUCTS_H

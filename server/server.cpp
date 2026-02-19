#include "server.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>
#include <tuple>
#include <vector>

#include "../USBMSCESP32-S2mini/structs.h"

void Server() {
  // minimal constructor
}

void Server::Init(const std::string& image_path) {
  fat32_ = std::make_unique<FAT32_IMG>();
  if (!fat32_->Init(image_path)) {
    std::cerr << "Error: Failed to initialize FAT32 image" << std::endl;
    exit(EXIT_FAILURE);
  }
  auto a = fat32_->get_image_sector_size_and_total_sectors_32();
  std::cout << "Sector size: " << a.first << std::endl;
  std::cout << "Total sectors: " << a.second << std::endl;
}

void Server::handleSetRequest(const sectors_buffer& request) {
  fat32_->write(request.lba, request.offset, request.buffer, request.bufsize);
}

int Server::doReceive(int client_socket, char* buffer, int length) {
  int bytes_recv = recv(client_socket, buffer, length, 0);

  // std::cout << std::chrono::system_clock::now().time_since_epoch().count() <<
  //     " Received " << bytes_recv << " bytes" << std::endl;
  if (bytes_recv <= 0) {
    return -1;
  }
  if (bytes_recv != sizeof(remote_request) &&
      bytes_recv != sizeof(sectors_buffer)) {
    // it's part of the sectors_buffer, waiting for the rest of the data
    int remaining_bytes = sizeof(sectors_buffer) - bytes_recv;
    char* buffer_ptr = buffer + bytes_recv;
    while (remaining_bytes > 0) {
      int bytes_recv2 = recv(client_socket, buffer_ptr, remaining_bytes, 0);
      if (bytes_recv2 <= 0) {
        break;
        return -1;
      }
      remaining_bytes -= bytes_recv2;
      buffer_ptr += bytes_recv2;

      std::cout << std::chrono::system_clock::now().time_since_epoch().count()
                << " Received " << bytes_recv2
                << " bytes, remaining: " << remaining_bytes << std::endl;
      bytes_recv = sizeof(sectors_buffer) - remaining_bytes;
    }
  }
  return bytes_recv;
}

bool Server::handleReadRequest(int client_socket, const char* buffer) {
  remote_request request;
  std::memcpy(&request, buffer, sizeof(request));
  if (request.length == 0) {
    std::vector<uint8_t> answer_data;
    answer_data.resize(sizeof(init_answer));  // HERE
    auto* answer = (init_answer*)&answer_data[0];
    std::tie(answer->sector_size, answer->sectors_count) =
        fat32_->get_image_sector_size_and_total_sectors_32();
    std::cout << "Client: Init request" << std::endl;
    if (send(client_socket, (char*)&answer_data[0], answer_data.size(), 0) !=
        answer_data.size()) {
      std::cout << "[Error] Cant send data to client" << std::endl;
      return false;
    }
    std::cout << "Sent init answer: sector_size=" << answer->sector_size
              << ", sectors_count=" << answer->sectors_count << std::endl;
  } else {
    std::vector<uint8_t> buffer;
    if (request.length > 16777216) {
      std::cout << "Error: request.length > 16777216" << std::endl;
      return false;
    }
    buffer.resize(request.length);
    std::cout << "Reading " << request.length << " bytes from position "
              << request.position << std::endl;
    if (!fat32_->read(request.position / 512, request.position % 512,
                      buffer.data(), request.length)) {
      std::cout << "Error: Can't read from image" << std::endl;
      return false;
    }
    if (send(client_socket, (char*)&buffer[0], request.length, 0) !=
        request.length) {
      std::cout << "[Error] Cant send data to client" << std::endl;
      return false;
    }
  }
  return true;
}

bool Server::handleWriteRequest(const char* buffer) {
  sectors_buffer request;
  std::memcpy(&request, buffer, sizeof(request));
  if (request.status == BUFFER_STATUS_NEED_REQUEST_WRITE) {
    std::cout << std::chrono::system_clock::now().time_since_epoch().count()
              << " Write request (lba=" << request.lba
              << ", offset=" << request.offset << ", size=" << request.bufsize
              << ")" << std::endl;
    handleSetRequest(request);
  } else {
    std::cerr << "[ERROR] Write request error; Status=" << request.status
              << std::endl;
    return false;
  }
  return true;
}

void Server::handleClient(int client_socket) {
  std::cout << "handleClient: " << client_socket << " Connection accepted"
            << std::endl;
  int optval = 1;
  socklen_t optlen = sizeof(optval);
  int res =
      setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
  if (res < 0) {
    std::cout << "Error: Can't set SO_KEEPALIVE option" << std::endl;
    exit(EXIT_FAILURE);
  }

  res = setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
  if (res < 0) {
    std::cout << "Error: Can't set SO_REUSEADDR option" << std::endl;
    exit(EXIT_FAILURE);
  }

  while (true) {
    bool closed = false;
    char max_buffer[sizeof(sectors_buffer)];
    int max_length = sizeof(max_buffer);

    int bytes_recv = doReceive(client_socket, max_buffer, max_length);
    if (bytes_recv <= 0) {
    } else {
      std::cout << std::chrono::system_clock::now().time_since_epoch().count()
                << " doReceive returned " << bytes_recv << " bytes"
                << std::endl;
      if (bytes_recv == sizeof(remote_request)) {
        closed = !handleReadRequest(client_socket, max_buffer);
      } else {
        closed = !handleWriteRequest(max_buffer);
      }
    }
    if (closed) {
      break;
    }
  }
  close(client_socket);
  std::cout << "handleClient " << client_socket << " : Connection closed"
            << std::endl;
}

void Server::Run() {
  int server_fd, new_socket, valread;
  struct sockaddr_in address = {};
  int opt = 1;
  int addrlen = sizeof(address);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == 0) {
    std::cout << "Error: Create socket failed" << std::endl;
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(12345);

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    std::cerr << "Error: setsockopt SO_REUSEADDR failed" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    std::cerr << "Error: bind failed" << std::endl;
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0) {
    std::cerr << "Error: listen failed" << std::endl;
    exit(EXIT_FAILURE);
  }
  while ((new_socket = accept(server_fd, (struct sockaddr*)&address,
                              reinterpret_cast<socklen_t*>(&addrlen))) >= 0) {
    handleClient(new_socket);
  }
}

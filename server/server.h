#ifndef SERVER_H
#define SERVER_H

#include <memory>

#include "../USBMSCESP32-S2mini/structs.h"
#include "../fat32/fat32_img.h"

class IFAT32;

class Server {
 public:
  void Init(const std::string& image_path);
  void Run();
  void Stop();

 private:
  void handleClient(int client_socket);
  void handleSetRequest(const sectors_buffer& request);
  static int doReceive(int client_socket, char* buffer, int length);
  bool handleReadRequest(int client_socket, const char* buffer);
  bool handleWriteRequest(const char* buffer);
  std::unique_ptr<IFAT32> fat32_;
};

#endif  // SERVER_H

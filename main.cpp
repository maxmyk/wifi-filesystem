#include "server/server.h"

int main(int argc, char* argv[]) {
  std::string image_path = "test.img";
  if (argc > 1) {
    image_path = argv[1];
  }
  Server server;
  server.Init(image_path);
  server.Run();
  return 0;
}
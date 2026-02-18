// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <fstream>
#include <tuple>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include "USBMSCESP32-S2mini/structs.h"

#pragma pack(push,1)

//FAT32 Boot Sector, bytes 0-35
struct BPB_FAT {
    uint8_t jump[3]; // 0x00
    char oem_name[8]; // 0x03
    uint16_t sector_size; // 0x0B
    uint8_t sectors_per_cluster; // 0x0D
    uint16_t reserved_sectors; // 0x0E
    uint8_t num_fats; // 0x10
    uint16_t root_entries; // 0x11
    uint16_t total_sectors_16; // 0x13
    uint8_t media_type; // 0x15
    uint16_t fat_size_16; // 0x16
    uint16_t sectors_per_track; // 0x18
    uint16_t num_heads; // 0x1A
    uint32_t hidden_sectors; // 0x1C
    uint32_t total_sectors_32; // 0x20
};

#pragma pack(pop)

std::pair<uint16_t,uint32_t> read_fat32_info(const char* image_file) {
    std::ifstream file(image_file, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return std::make_pair(0,0);
    }

    BPB_FAT bpb{};
    file.read((char*)&bpb, sizeof(bpb));
    std::cout << "Reading FAT32 info from: " << image_file << std::endl;
    std::cout << "  uint8_ts per Sector: " << std::dec << bpb.sector_size << std::endl;
    std::cout << "  Total Sectors (32-bit): " << std::dec << bpb.total_sectors_32 << std::endl;
    return std::make_pair(bpb.sector_size, bpb.total_sectors_32);
}

std::vector<uint8_t> get_init_answer()
{
    std::vector<uint8_t> answer_data;
    answer_data.resize(sizeof(init_answer)); // HERE
    auto* answer = (init_answer*)&answer_data[0];
    std::tie(answer->sector_size,answer->sectors_count) = read_fat32_info("test.img");
    return answer_data;
}

void handleClientRead(int client_socket) {
    //std::cout << "Client: Connection accepted" << std::endl;
    int optval = 1;
    socklen_t optlen = sizeof(optval);
    int res = setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
    if (res < 0)
    {
        //std::cout << "Error: Can't set SO_KEEPALIVE option" << std::endl;
        exit(EXIT_FAILURE);
    }

    // reuse address
    res = setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    if (res < 0)
    {
        //std::cout << "Error: Can't set SO_REUSEADDR option" << std::endl;
        exit(EXIT_FAILURE);
    }

    //std::cout << "Client: Connection accepted" << std::endl;

    while (true)
    {
        bool closed = false;
        remote_request request = {};
        char *buffer = (char *) &request;
        int length = sizeof(request);
        while (length > 0) {
            int uint8_ts_recv = recv(client_socket, buffer, length, 0);
            if (uint8_ts_recv <= 0) {
                closed = true;
                break;
            }
            length -= uint8_ts_recv;
            buffer += uint8_ts_recv;
        }
        if (closed) {
            break;
        }
        if (request.length == 0) {
            std::vector<uint8_t> answer_data = get_init_answer();
            //std::cout << "Client: Init request" << std::endl;

            if (send(client_socket, (char *) &answer_data[0], answer_data.size(), 0) != answer_data.size()) {
                //std::cout << "Error: Cant send data to client" << std::endl;
                closed = true;
            }
        } else {
            //std::cout << "Client: Data request (addr=" << request.position << ", size=" << request.length << ")"
                      //<< std::endl;

            std::vector<uint8_t> buffer;
            if (request.length > 16777216) {
                //std::cout << "Error: request.length > 16777216" << std::endl;
                closed = true;
            }
            buffer.resize(request.length);
            // =====================
            std::cout << "Reading " << request.length << " bytes from position " << request.position
                      << std::endl;
            std::ifstream file("test.img", std::ios::binary);
            file.seekg(request.position);
            char buffer2[request.length];
            file.read(buffer2, request.length);
            for (int i = 0; i < request.length; i++) {
                buffer[i] = static_cast<uint8_t>(buffer2[i]);
            }
            //std::cout << "Buffer: " << std::endl;
            for (int i = 0; i < request.length; i++) {
                //std::cout << buffer2[i];
            }
            // =====================
            if (send(client_socket, (char *) &buffer[0], request.length, 0) != request.length) {
                //std::cout << "Error: Cant send data to client" << std::endl;
                closed = true;
            } else {
                //std::cout << "Client: Data sent" << std::endl;
            }
        }
        if (closed)
        {
            break;
        }
    }
    close(client_socket);
    //std::cout << "Client: Connection closed" << std::endl;
}

int main(int argc, char** argv) {
    //std::cout << sizeof(sectors_buffer) << std::endl; // 32796
    //std::cout << sizeof(remote_request) << std::endl; // 12
    //std::cout << sizeof(init_answer) << std::endl; // 24

    int server_fd, new_socket, valread;
    struct sockaddr_in address = {};
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == 0)
    {
        //std::cout << "Error: Create socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(12345);
    // reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        //std::cout << "Error: Can't set SO_REUSEADDR option" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        //std::cout << "Error: Can't bind on port " << 12345 << std::endl;
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        //std::cout << "Error: Can't listen on port " << 12345 << std::endl;
        exit(EXIT_FAILURE);
    }

    //std::cout << "Server: Started on port " << 12345 << std::endl;
    while ((new_socket = accept(server_fd, (struct sockaddr*)&address, reinterpret_cast<socklen_t *>(&addrlen))) >= 0) {
        // std::thread(handleClient, new_socket).detach();
        handleClientRead(new_socket);
    }
}

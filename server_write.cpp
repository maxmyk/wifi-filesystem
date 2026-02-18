// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include "USBMSCESP32-S2mini/structs.h"

void handleSetRequest(const sectors_buffer& request, const std::string& file_name = "test.img") {
    std::ofstream file(file_name.c_str(), std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) {
        std::cerr << "Error: Can't open file " << file_name << std::endl;
    }
    uint64_t pos = (uint64_t)request.lba * 512 + request.offset;
//    file.seekp(request.lba * BUFFER_SIZE + request.offset);
    file.seekp(pos);
    file.write((char*)request.buffer, request.bufsize);
    file.close();
}

void handleClientWrite(int client_socket) {
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
        sectors_buffer request = {};
        char *buffer = (char *) &request;
        int length = sizeof(request);
        while (length > 0) {
            int bytes_recv = recv(client_socket, buffer, length, 0);
            if (bytes_recv <= 0) {
                closed = true;
                break;
            }
            length -= bytes_recv;
            buffer += bytes_recv;
        }
        if (request.status == BUFFER_STATUS_NEED_REQUEST) {
            //std::cout << "Client: Request received" << std::endl;
            //std::cout << "Client: Request lba: " << request.lba << std::endl;
            //std::cout << "Client: Request bufsize: " << request.bufsize << std::endl;
            //std::cout << "Client: Request offset: " << request.offset << std::endl;
            //std::cout << "Client: Request recv_offset: " << request.recv_offset << std::endl;
            //std::cout << "Client: Request recv_len: " << request.recv_len << std::endl;
            //std::cout << "Client: Request buffer: " << std::endl;
            for (int i = 0; i < request.bufsize; i++) {
                //std::cout << (int)request.buffer[i] << " ";
            }
            //std::cout << std::endl;
            handleSetRequest(request);
            //std::cout << "Client: Request done" << std::endl;
        } else {
            std::cerr << "Client: Request error" << std::endl;
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
    address.sin_port = htons(12346);
    // reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        //std::cout << "Error: Can't set SO_REUSEADDR option" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        //std::cout << "Error: Can't bind on port " << 12346 << std::endl;
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        //std::cout << "Error: Can't listen on port " << 12346 << std::endl;
        exit(EXIT_FAILURE);
    }
    //std::cout << "Server: Started on port " << 12346 << std::endl;
    while ((new_socket = accept(server_fd, (struct sockaddr*)&address, reinterpret_cast<socklen_t *>(&addrlen))) >= 0) {
        // std::thread(handleClient, new_socket).detach();
        handleClientWrite(new_socket);
    }
}

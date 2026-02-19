cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME "wifi_usb_drive_server")

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
endif ()

# O3 optimization level
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

add_executable(${PROJECT_NAME}
        USBMSCESP32-S2mini/structs.h
        server/server.cpp
        server/server.h
        fat32/ifat32.h
        fat32/fat32_img.h
        fat32/fat32_img.cpp
        main.cpp
)
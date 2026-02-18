# not operational yet

#cmake_minimum_required(VERSION 3.23)
#project(OS_Project_MPZ)
#
#set(CMAKE_CXX_STANDARD 17)
#
#if (APPLE)
#    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
#endif ()
#
#add_executable(OS_Project_MPZ
#        USBMSCESP32-S2mini/structs.h
#        server/server.cpp
#        server/server.h
#        fat32/ifat32.h
#        fat32/FAT32_IMG.h
#        fat32/FAT32_IMG.cpp
#        main.cpp
#)
##add_executable(OS_Project_MPZ_write server_write.cpp)
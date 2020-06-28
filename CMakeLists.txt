cmake_minimum_required(VERSION 3.0)
project(FsDump)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

add_executable(fsdump
Apfs.cpp
Apfs.h
AppleSparseimage.cpp
AppleSparseimage.h
Crc32.cpp
Crc32.h
Device.h
DeviceLinux.cpp
DeviceLinux.h
FileSystem.h
GptPartitionMap.cpp
GptPartitionMap.h
main.cpp
)

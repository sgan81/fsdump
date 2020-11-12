cmake_minimum_required(VERSION 3.0)
project(FsDump)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_STANDARD 11)

add_executable(fsdump
Apfs.cpp
Apfs.h
apfs_layout.h
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
MasterBootRecord.h
Ntfs.cpp
Ntfs.h
ntfs_layout.h
Vhdx.cpp
Vhdx.h
)
if(APPLE)
target_include_directories(fsdump PRIVATE compat)
endif()
target_compile_definitions(fsdump PUBLIC _FILE_OFFSET_BITS=64 _DARWIN_USE_64_BIT_INODE)

# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(RedisClone VERSION 1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include the include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add server executable
add_executable(server src/server.cpp src/hashtable.cpp)

# Add client executable
add_executable(client src/client.cpp)

# Optional: Set output directory for the executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# Optional: Enable warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(server PRIVATE -Wall -Wextra)
    target_compile_options(client PRIVATE -Wall -Wextra)
endif()

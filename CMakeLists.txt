# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(RedisClone VERSION 1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Source directory
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Get all source files in the src directory
file(GLOB_RECURSE ALL_SOURCES "${SRC_DIR}/*.cpp")

# Create a list for server sources by removing client.cpp
set(SERVER_SOURCES ${ALL_SOURCES})
list(FILTER SERVER_SOURCES EXCLUDE REGEX ".*client\\.cpp$")

# Include the include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Create server executable
add_executable(server ${SERVER_SOURCES})

# Create client executable from just client.cpp
add_executable(client ${SRC_DIR}/client.cpp)

# Optional: Set output directory for the executable
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# Optional: Enable warnings and AddressSanitizer
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(server PRIVATE -Wall -Wextra -fsanitize=address -g -fno-omit-frame-pointer)
    target_compile_options(client PRIVATE -Wall -Wextra -fsanitize=address -g -fno-omit-frame-pointer)

    target_link_options(server PRIVATE -fsanitize=address)
    target_link_options(client PRIVATE -fsanitize=address)
endif()


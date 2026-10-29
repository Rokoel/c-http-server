# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.15)

# Name your project and specify the programming language
project(MyCProject LANGUAGES C)

# Force a C standard (e.g., C11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Define your executable name and the source files needed to build it
add_executable(server core/core.c core/get_filepath.c core/helpful.c core/server.c core/socket.c core/split_buffer.c)
target_include_directories(server PRIVATE "server/core")
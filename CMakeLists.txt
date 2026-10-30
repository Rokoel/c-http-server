# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.15)

# Name your project and specify the programming language
project(MyCProject LANGUAGES C)

set(ROKOEL_VIEWS_FOLDER "../public/views" CACHE STRING
    "Filesystem path containing the website views")
set(ROKOEL_LOGS_FOLDER "../logs" CACHE STRING
    "Filesystem path containing runtime logs")

# Force a C standard (e.g., C11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Define your executable name and the source files needed to build it
add_executable(server core/core.c core/get_filepath.c core/helpful.c core/server.c core/socket.c core/split_buffer.c)
find_package(Threads REQUIRED)
target_include_directories(server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core")
target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(server PRIVATE
    ROKOEL_VIEWS_FOLDER="${ROKOEL_VIEWS_FOLDER}"
    ROKOEL_LOGS_FOLDER="${ROKOEL_LOGS_FOLDER}"
)
target_link_libraries(server PRIVATE Threads::Threads)

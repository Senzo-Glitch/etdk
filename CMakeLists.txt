cmake_minimum_required(VERSION 3.15)
project(disknuke VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(PLATFORM_MACOS)
elseif(UNIX)
    add_compile_definitions(PLATFORM_LINUX)
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.c
    src/crypto.c
    src/secure_delete.c
    src/platform.c
)

# Executable
add_executable(disknuke ${SOURCES})

# OpenSSL for cryptography
find_package(OpenSSL REQUIRED)
target_link_libraries(disknuke OpenSSL::Crypto)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(disknuke pthread)
endif()

# Installation
install(TARGETS disknuke DESTINATION bin)

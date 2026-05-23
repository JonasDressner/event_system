cmake_minimum_required(VERSION 3.10)
project(EventSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(event_system
    src/main.cpp
    src/producer.cpp
    src/consumer.cpp
)

# Platform-specific settings
if(WIN32)
    message(STATUS "Building for Windows")
else()
    message(STATUS "Building for Linux")
    target_link_libraries(event_system pthread)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /std:c++17)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

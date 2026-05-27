cmake_minimum_required(VERSION 3.11)
project(EventSystemProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generates build/compile_commands.json — needed by clang-tidy and IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Optional: clang-tidy static analysis ────────────────────────────
# Enable with: cmake -DENABLE_CLANG_TIDY=ON ..
option(ENABLE_CLANG_TIDY "Run clang-tidy on all sources during build" OFF)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-17 clang-tidy-16)
    if(CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy not found — install LLVM: winget install LLVM.LLVM")
    endif()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(EventSystemLib
    src/event.cpp
    src/event_serializer.cpp
    src/producer.cpp
    src/consumer.cpp
    src/ipc_factory.cpp
    src/ipc.cpp
)

add_executable(EventSystem
    src/main.cpp
)

target_link_libraries(EventSystem PRIVATE EventSystemLib)

# Platform-specific settings
if(WIN32)
    message(STATUS "Building for Windows")
else()
    message(STATUS "Building for Linux")
    target_link_libraries(EventSystem PRIVATE pthread)
endif()

# ── Compiler warnings — only for own targets ────────────────────────
if(MSVC)
    target_compile_options(EventSystemLib PRIVATE /W4)
    target_compile_options(EventSystem    PRIVATE /W4)
else()
    target_compile_options(EventSystemLib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(EventSystem    PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Optional: unit tests ─────────────────────────────────────────────
# Disable with: cmake -DBUILD_TESTS=OFF ..
option(BUILD_TESTS "Build unit tests (requires internet for Catch2)" ON)

if(BUILD_TESTS)
    include(FetchContent)
    include(CTest)
    enable_testing()

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)
    include(Catch)

    add_executable(EventSystemTests
        test/event_test.cpp
        test/producer_test.cpp
        test/consumer_test.cpp
    )

    target_link_libraries(EventSystemTests PRIVATE Catch2::Catch2WithMain EventSystemLib)
    target_include_directories(EventSystemTests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/test
    )

    if(MSVC)
        target_compile_options(EventSystemTests PRIVATE /W4)
    else()
        target_compile_options(EventSystemTests PRIVATE -Wall -Wextra -Wpedantic)
        if(NOT WIN32)
            target_link_libraries(EventSystemTests PRIVATE pthread)
        endif()
    endif()

    catch_discover_tests(EventSystemTests
        TEST_PREFIX "Test/"
    )

    message(STATUS "Unit tests: ENABLED")
else()
    message(STATUS "Unit tests: DISABLED  (cmake -DBUILD_TESTS=ON .. to enable)")
endif()

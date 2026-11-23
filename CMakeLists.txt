cmake_minimum_required(VERSION 3.20)

project(Niemu
    VERSION 0.1.0
    DESCRIPTION "Nice Emulator - independent AArch64 full-system emulator"
    LANGUAGES CXX
)

option(NIEMU_BUILD_TESTS "Build Niemu native tests" ON)

add_library(niemu_core STATIC
    core/src/version.cpp
    core/src/cpu/cpu_state.cpp
    core/src/cpu/system_registers.cpp
    core/src/cpu/decoder.cpp
    core/src/cpu/interpreter.cpp
    core/src/memory/memory_bus.cpp
    core/src/memory/address_space.cpp
)

target_include_directories(niemu_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>
)

target_compile_features(niemu_core PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(niemu_core PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
    )
endif()

if(NIEMU_BUILD_TESTS)
    enable_testing()
    add_executable(niemu_core_tests tests/core_tests.cpp)
    target_link_libraries(niemu_core_tests PRIVATE niemu_core)
    target_compile_features(niemu_core_tests PRIVATE cxx_std_20)
    add_test(NAME niemu_core_tests COMMAND niemu_core_tests)
endif()

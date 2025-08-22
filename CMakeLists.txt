cmake_minimum_required(VERSION 3.18)
project(aurora LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM (config mode)
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")
message(STATUS "LLVM targets: ${LLVM_TARGETS_TO_BUILD}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Compiler
file(GLOB AURORA_SRC
  src/*.cpp
)
add_executable(aurorac ${AURORA_SRC})
target_include_directories(aurorac PRIVATE include src)
target_compile_definitions(aurorac PRIVATE -D_GNU_SOURCE)
# Link a minimal set. Adjust for your LLVM version.
llvm_map_components_to_libnames(LLVM_LIBS
    core orcjit mcjit native nativecodegen ipo irreader
    support mc mcparser target targetparser transformutils passes)
target_link_libraries(aurorac PRIVATE ${LLVM_LIBS})

# Runtime
add_library(aurora_runtime OBJECT stdlib/aurora_runtime.c)

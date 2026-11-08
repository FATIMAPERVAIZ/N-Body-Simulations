cmake_minimum_required(VERSION 3.16)
project(DistributedNBody VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# No -ffast-math and no -march=native, so results stay reproducible
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(MPI REQUIRED COMPONENTS CXX)

# Code shared by the sequential and MPI programs
add_library(nbody_common STATIC
    src/common/configuration.cpp
    src/common/initialization.cpp
    src/common/physics.cpp
    src/common/simulation.cpp
    src/common/io.cpp
    src/common/barnes_hut.cpp)
# Sequential baseline
add_executable(nbody_seq src/sequential/nbody_seq.cpp)
target_link_libraries(nbody_seq PRIVATE nbody_common)

# MPI distributed version
add_executable(nbody_mpi src/mpi/nbody_mpi.cpp)
target_link_libraries(nbody_mpi PRIVATE nbody_common MPI::MPI_CXX)

# Stage 6: sequential vs MPI state comparison tool
add_executable(compare_states tools/compare_states.cpp)

# Tests
enable_testing()
add_executable(test_physics tests/test_physics.cpp)
target_link_libraries(test_physics PRIVATE nbody_common)
add_test(NAME physics COMMAND test_physics)

# ... existing targets ...

add_executable(nbody_seq_bh src/sequential/nbody_seq_bh.cpp)
target_link_libraries(nbody_seq_bh PRIVATE nbody_common)

add_executable(nbody_mpi_bh src/mpi/nbody_mpi_bh.cpp)
target_link_libraries(nbody_mpi_bh PRIVATE nbody_common MPI::MPI_CXX)

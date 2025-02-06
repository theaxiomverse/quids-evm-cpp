cmake_minimum_required(VERSION 3.16)
project(qzkp_core)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Set OpenSSL paths
set(OPENSSL_ROOT_DIR "/usr/local/Cellar/openssl@3/3.4.0")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib")

find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(fmt REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)

# Set JSON include directory
set(JSON_INCLUDE_DIR "/usr/local/include")
if(NOT EXISTS "${JSON_INCLUDE_DIR}/nlohmann/json.hpp")
    message(FATAL_ERROR "nlohmann/json.hpp not found. Please install nlohmann-json library.")
endif()

# Set Eigen3 configuration
set(EIGEN3_INCLUDE_DIR "/usr/local/include/eigen3")
if(NOT EXISTS ${EIGEN3_INCLUDE_DIR})
    message(FATAL_ERROR "Eigen3 include directory not found at ${EIGEN3_INCLUDE_DIR}")
endif()

# Create interface target for Eigen3 if not found
if(NOT TARGET Eigen3::Eigen)
    add_library(Eigen3::Eigen INTERFACE IMPORTED)
    set_target_properties(Eigen3::Eigen PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${EIGEN3_INCLUDE_DIR}"
    )
endif()

# Find BLAKE3 manually
find_path(BLAKE3_INCLUDE_DIR blake3.h
    PATHS /usr/local/include
    REQUIRED
)
find_library(BLAKE3_LIBRARY
    NAMES blake3
    PATHS /usr/local/lib
    REQUIRED
)

# Find zstd manually
find_path(ZSTD_INCLUDE_DIR zstd.h
    PATHS /usr/local/include
    REQUIRED
)
find_library(ZSTD_LIBRARY
    NAMES zstd
    PATHS /usr/local/lib
    REQUIRED
)

# Find RocksDB
set(ROCKSDB_ROOT "/usr/local/Cellar/rocksdb/9.10.0")
set(ROCKSDB_INCLUDE_DIR "${ROCKSDB_ROOT}/include")
set(ROCKSDB_LIBRARY_DIR "${ROCKSDB_ROOT}/lib")

include_directories(${ROCKSDB_INCLUDE_DIR})

find_library(ROCKSDB_LIBRARY
    NAMES rocksdb
    PATHS ${ROCKSDB_LIBRARY_DIR}
    REQUIRED
)

if(NOT ROCKSDB_LIBRARY)
    message(FATAL_ERROR "RocksDB library not found")
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# Source files
file(GLOB_RECURSE SOURCES
    "src/blockchain/*.cpp"
    "src/quantum/*.cpp"
    "src/zkp/*.cpp"
    "src/storage/*.cpp"
    "src/rollup/*.cpp"
    "src/evm/*.cpp"
)

# Create shared library
add_library(qzkp_core SHARED ${SOURCES})
target_link_libraries(qzkp_core
    PRIVATE
    OpenMP::OpenMP_CXX
    OpenSSL::Crypto
    OpenSSL::SSL
    CURL::libcurl
    ${ZSTD_LIBRARY}
    ${BLAKE3_LIBRARY}
    ${ROCKSDB_LIBRARY}
    Eigen3::Eigen
    fmt::fmt
    ${ZLIB_LIBRARIES}
)

# Modern CMake way to handle includes
target_include_directories(qzkp_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${ROCKSDB_INCLUDE_DIR}
        ${JSON_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${EIGEN3_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${ZSTD_INCLUDE_DIR}
        ${BLAKE3_INCLUDE_DIR}
        ${CURL_INCLUDE_DIRS}
)

# Enable testing
enable_testing()

# Test files
file(GLOB_RECURSE TEST_SOURCES
    "tests/rollup/*.cpp"
    "tests/blockchain/*.cpp"
    "tests/quantum/*.cpp"
    "tests/zkp/*.cpp"
)

# Create test executable
add_executable(enhanced_ml_tests ${TEST_SOURCES})
target_link_libraries(enhanced_ml_tests
    PRIVATE
    qzkp_core
    GTest::GTest
    GTest::Main
    fmt::fmt
)

target_include_directories(enhanced_ml_tests
    PRIVATE
    ${JSON_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)

# Add test
add_test(
    NAME enhanced_ml_tests
    COMMAND enhanced_ml_tests
)

# Add separate high performance benchmark target
add_executable(rollup_benchmarks
    tests/rollup/RollupBenchmarkTests.cpp
)

target_link_libraries(rollup_benchmarks
    PRIVATE
    qzkp_core
    GTest::GTest
    GTest::Main
    fmt::fmt
    OpenMP::OpenMP_CXX
    OpenSSL::Crypto
    OpenSSL::SSL
)

target_include_directories(rollup_benchmarks
    PRIVATE
    ${JSON_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)

# Enable maximum optimization for benchmarks
target_compile_options(rollup_benchmarks PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-O3 -march=native -mtune=native>
    $<$<CXX_COMPILER_ID:Clang>:-O3 -march=native -mtune=native>
    $<$<CXX_COMPILER_ID:AppleClang>:-O3 -march=native -mtune=native>
)

if(ENABLE_LTO)
    set_property(TARGET rollup_benchmarks PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Installation
install(TARGETS qzkp_core
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h") 
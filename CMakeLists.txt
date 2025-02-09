cmake_minimum_required(VERSION 3.20...3.31)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(qzkp_core)

# Add module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Find required packages
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Remove libp2p dependency
# find_package(libp2p REQUIRED)
# if(NOT libp2p_FOUND)
#     message(FATAL_ERROR "libp2p not found. Please install libp2p library.")
# endif()

find_package(GTest REQUIRED)
find_package(OpenSSL 3.0 REQUIRED)
find_package(fmt REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(BLAKE3 REQUIRED)



# Add OpenSSL include directories
include_directories(${OPENSSL_INCLUDE_DIR})

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

# Add OpenSSL include directories
include_directories(SYSTEM ${OPENSSL_INCLUDE_DIR})

# Find OpenSSL package
find_package(OpenSSL 3.0 REQUIRED)

# Add binary dir to include path for generated header
include_directories(${CMAKE_BINARY_DIR}/include)

# Set C++ flags for OpenSSL
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${OPENSSL_INCLUDE_DIR}")

# Add OpenSSL to compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendors/includes")

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

# Find GMP
set(GMP_ROOT "/usr/local/Cellar/gmp/6.3.0")
set(GMP_INCLUDE_DIR "${GMP_ROOT}/include")
set(GMP_LIBRARY_DIR "${GMP_ROOT}/lib")

find_library(GMP_LIBRARY
    NAMES gmp
    PATHS ${GMP_LIBRARY_DIR}
    REQUIRED
)

if(NOT GMP_LIBRARY)
    message(FATAL_ERROR "GMP library not found")
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# Source files
file(GLOB_RECURSE SOURCES
    "src/blockchain/*.cpp"
    "src/crypto/*.cpp"
    "src/quantum/*.cpp"
    "src/zkp/*.cpp"
    "src/storage/*.cpp"
    "src/rollup/*.cpp"
    "src/evm/*.cpp"
    "src/rollup/StateTransitionProof.cpp"
)

# Find spdlog before using it
find_package(spdlog REQUIRED)

# Organize source files by component
set(CLI_SOURCES
    src/cli/QuidsCommand.cpp
    src/cli/QuidsCLI.cpp
    src/cli/commands/StartCommand.cpp
    src/cli/commands/StopCommand.cpp
    src/cli/commands/StatusCommand.cpp
)

set(NODE_SOURCES
    src/node/QuidsNode.cpp
)

set(CONTROL_SOURCES
    src/control/QuidsControl.cpp
)

# Combine all sources
set(QUIDS_SOURCES
    ${CLI_SOURCES}
    ${NODE_SOURCES}
    ${CONTROL_SOURCES}
    ${SOURCES}  # Keep existing sources
)

# Modify the existing quids_core library
add_library(quids_core SHARED ${QUIDS_SOURCES})

# Add core dependencies
target_link_libraries(quids_core
    PRIVATE
    OpenMP::OpenMP_CXX
    OpenSSL::Crypto
    OpenSSL::SSL
    CURL::libcurl
    ${ZSTD_LIBRARY}
    ${BLAKE3_LIBRARY}
    ${ROCKSDB_LIBRARY}
    ${GMP_LIBRARY}
    Eigen3::Eigen
    fmt::fmt
    ${ZLIB_LIBRARIES}
    spdlog::spdlog
    quids_network
)

# Modern CMake way to handle includes
target_include_directories(quids_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${ROCKSDB_INCLUDE_DIR}
        ${JSON_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIR}
        ${VENDOR_DIR}
        ${GMP_INCLUDE_DIR}
)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Enable testing
enable_testing()

# Test files
file(GLOB_RECURSE TEST_SOURCES
    "tests/rollup/*.cpp"
    "tests/blockchain/*.cpp"
    "tests/quantum/*.cpp"
    "tests/zkp/*.cpp"
)

# Add test files
set(TEST_SOURCES
    ${TEST_SOURCES}
    tests/blockchain/TransactionTest.cpp
    tests/evm/EVMExecutorTest.cpp
    tests/evm/uint256Test.cpp
)

# Create test executable
add_executable(enhanced_ml_tests ${TEST_SOURCES})
target_link_libraries(enhanced_ml_tests
    PRIVATE
    quids_core
    GTest::GTest
    GTest::Main
    fmt::fmt
    OpenSSL::Crypto
    OpenSSL::SSL
)

target_include_directories(enhanced_ml_tests
    PRIVATE
    ${JSON_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${VENDOR_DIR}
    ${FALCON_INCLUDE_DIR}
    ${SHA3_INCLUDE_DIR}
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
    quids_core
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
    ${VENDOR_DIR}
    ${FALCON_INCLUDE_DIR}
    ${SHA3_INCLUDE_DIR}
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
install(TARGETS quids_core
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp")

# Add Falcon include directory
set(FALCON_INCLUDE_DIR "${VENDOR_DIR}/falcon")
set(SHA3_INCLUDE_DIR "${VENDOR_DIR}/sha3")

target_include_directories(quids_core
    PRIVATE
    ${FALCON_INCLUDE_DIR}
    ${SHA3_INCLUDE_DIR}
)

# Add executable
add_executable(quids src/main.cpp)
target_link_libraries(quids PRIVATE quids_core)

# Add forward declarations header
install(FILES include/node/ForwardDeclarations.hpp
    DESTINATION include/node
)

# Add blockchain and EVM libraries
add_library(quids_blockchain SHARED
    src/blockchain/Chain.cpp
    src/blockchain/Block.cpp
    src/blockchain/Account.cpp
    src/blockchain/Transaction.cpp
)

add_library(quids_evm SHARED
    src/evm/EVMExecutor.cpp
    src/evm/Memory.cpp
    src/evm/Stack.cpp
    src/evm/Storage.cpp
    src/evm/uint256.cpp
    src/evm/Address.cpp
)

# Add include directories
target_include_directories(quids_blockchain PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(quids_evm
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(quids_blockchain PUBLIC
    quids_evm
    OpenSSL::SSL
    OpenSSL::Crypto
    BLAKE3::BLAKE3
)

target_link_libraries(quids_evm
    PUBLIC
    spdlog::spdlog
)

target_link_libraries(quids_core
    PRIVATE
    quids_blockchain
    quids_evm
)

# Add network library
add_library(quids_network SHARED
    src/network/P2PConnection.cpp
    src/network/P2PNode.cpp
)

target_include_directories(quids_network
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(quids_network
    PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
)

# Update quids_core dependencies
target_link_libraries(quids_core
    PRIVATE
    OpenMP::OpenMP_CXX
    OpenSSL::Crypto
    OpenSSL::SSL
    CURL::libcurl
    ${ZSTD_LIBRARY}
    ${BLAKE3_LIBRARY}
    ${ROCKSDB_LIBRARY}
    ${GMP_LIBRARY}
    Eigen3::Eigen
    fmt::fmt
    ${ZLIB_LIBRARIES}
    spdlog::spdlog
    quids_network
)

# Add P2P example executable
add_executable(p2p_example examples/p2p_example.cpp)
target_link_libraries(p2p_example
    PRIVATE
    quids_network
    spdlog::spdlog
) 
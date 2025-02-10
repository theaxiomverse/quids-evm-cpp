cmake_minimum_required(VERSION 3.20)
project(Quids LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies setup
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)
include_directories(${nlohmann_json_SOURCE_DIR}/include)

find_package(Eigen3 REQUIRED)
find_package(OpenMP REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
find_package(PkgConfig REQUIRED)

# Update Boost configuration
find_package(Boost 1.81 REQUIRED COMPONENTS system)



# Core dependencies
find_path(ZSTD_INCLUDE_DIR zstd.h
    PATHS /usr/local/include
    REQUIRED
)
find_library(ZSTD_LIBRARY
    NAMES zstd
    PATHS /usr/local/lib
    REQUIRED
)

# RocksDB setup
set(ROCKSDB_ROOT "/usr/local/Cellar/rocksdb/9.10.0")
set(ROCKSDB_INCLUDE_DIR "${ROCKSDB_ROOT}/include")
set(ROCKSDB_LIBRARY_DIR "${ROCKSDB_ROOT}/lib")
find_library(ROCKSDB_LIBRARY rocksdb PATHS ${ROCKSDB_LIBRARY_DIR} REQUIRED)

# GMP setup
set(GMP_ROOT "/usr/local/Cellar/gmp/6.3.0")
set(GMP_INCLUDE_DIR "${GMP_ROOT}/include")
set(GMP_LIBRARY_DIR "${GMP_ROOT}/lib")
find_library(GMP_LIBRARY gmp PATHS ${GMP_LIBRARY_DIR} REQUIRED)

# NAT-PMP setup
set(NATPMP_ROOT "/usr/local/Cellar/libnatpmp/20230423")
find_library(NATPMP_LIBRARY
    NAMES natpmp
    PATHS ${NATPMP_ROOT}/lib /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib
    NO_DEFAULT_PATH
    REQUIRED
)
find_path(NATPMP_INCLUDE_DIR
    NAMES natpmp.h natpmp/natpmp.h natpmp
    PATHS ${NATPMP_ROOT}/include /usr/local/include /usr/include
    NO_DEFAULT_PATH
    REQUIRED
)


# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# Source organization
file(GLOB_RECURSE SOURCES
    "src/blockchain/*.cpp"
    "src/crypto/*.cpp"
    "src/zkp/*.cpp"
    "src/storage/*.cpp"
    "src/rollup/*.cpp"
    "src/evm/*.cpp"

    "vendors/includes/falcon/*.cpp"
    "vendors/includes/sha3/*.cpp"
 
)

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

set(NETWORK_SOURCES

    src/network/P2PNode.cpp
    src/network/P2PConnection.cpp
 
)

set(COMMON_SOURCES
    src/common/Logger.cpp
    src/common/Config.cpp
)

set(QUANTUM_SOURCES
    src/quantum/QuantumState.cpp
  

)


# Main library
add_library(quids_core SHARED 
    ${SOURCES}
    ${CLI_SOURCES}
    ${NODE_SOURCES}
    ${CONTROL_SOURCES}
    ${NETWORK_SOURCES}
)

include_directories(
   
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/vendors/includes
    /usr/local/include
)

add_library(quids_quantum SHARED ${QUANTUM_SOURCES})

# Network library
add_library(quids_network SHARED ${NETWORK_SOURCES} src/network/STUNClient.cpp src/network/UPnPClient.cpp src/network/NATPMP.cpp)


# Update the miniupnpc configuration
find_package(PkgConfig REQUIRED)
pkg_check_modules(MINIUPNPC REQUIRED miniupnpc)

# Update library path for miniupnpc
find_library(MINIUPNPC_LIBRARY
    NAMES miniupnpc
    PATHS 
    /usr/local/lib
    /opt/homebrew/lib
    /usr/lib
    /usr/local/Cellar/miniupnpc/*/lib
    REQUIRED
)

find_path(MINIUPNPC_INCLUDE_DIR
    NAMES miniupnpc.h miniupnpc/miniupnpc.h miniupnpc
    PATHS
    /usr/local/include /usr/include /usr/local/Cellar/miniupnpc/*/include
 
    REQUIRED
)

# Common library
add_library(quids_common SHARED ${COMMON_SOURCES})
target_link_libraries(quids_common
    PRIVATE
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
)

# Update the network library configuration
target_include_directories(quids_network
    PRIVATE
    ${NATPMP_INCLUDE_DIR}
    ${MINIUPNPC_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(quids_network
    PRIVATE
    ${NATPMP_LIBRARY}
    ${MINIUPNPC_LIBRARY}
    Boost::system
    fmt::fmt
    quids_common
)

target_link_libraries(quids_quantum
    PRIVATE
    ${NATPMP_LIBRARY}
    ${MINIUPNPC_LIBRARY}
    Eigen3::Eigen
    Boost::system
    fmt::fmt
    quids_common

)



# Find BLAKE3 library
find_library(BLAKE3_LIBRARY
    NAMES blake3
    PATHS
    /usr/local/lib
    /opt/homebrew/lib
    /usr/lib
    REQUIRED
)

find_path(BLAKE3_INCLUDE_DIR
    NAMES blake3.h blake3/blake3.h blake3
    PATHS
    /usr/local/include
    /opt/homebrew/include
    /usr/include
    REQUIRED
)

# Add include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    /usr/local/include
)

# Define the EVM library
add_library(quids_evm

    src/evm/Address.cpp
    src/evm/Compression.cpp
    src/evm/EVMExecutor.cpp
    src/evm/ExternalLink.cpp
    src/evm/uint256.cpp
    src/evm/FloatingPoint.cpp
    src/evm/Memory.cpp
    src/evm/SolidityParser.cpp
    src/evm/Stack.cpp
    src/evm/Storage.cpp
  
    )

# Set include directories specifically for quids_evm target
target_include_directories(quids_evm PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    /usr/local/include
)


# Set C++ standard for the target
target_compile_features(quids_evm PRIVATE cxx_std_20)


# Blockchain components
add_library(quids_blockchain SHARED
    src/blockchain/Chain.cpp
    src/blockchain/Block.cpp
    src/blockchain/Account.cpp
    src/blockchain/Transaction.cpp
)

# Add dependencies for blockchain library
target_link_libraries(quids_blockchain
    PRIVATE
    OpenSSL::Crypto
    OpenSSL::SSL
    fmt::fmt
    quids_evm
    ${BLAKE3_LIBRARY}  # Add BLAKE3 library
)

# Add include directories for blockchain
target_include_directories(quids_blockchain
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${BLAKE3_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)


# Update target dependencies
add_dependencies(quids_blockchain quids_quantum quids_network quids_common)


# Dependencies
target_link_libraries(quids_core
    PRIVATE
    OpenMP::OpenMP_CXX
    OpenSSL::Crypto
    OpenSSL::SSL
    CURL::libcurl
    ${ZSTD_LIBRARY}
    ${ROCKSDB_LIBRARY}
    ${GMP_LIBRARY}
    Eigen3::Eigen
    fmt::fmt
    spdlog::spdlog
    quids_quantum
    quids_network
    quids_blockchain
  
    ${MINIUPNPC_LIBRARY}  # Add miniupnpc
    ${NATPMP_LIBRARY}     # Add natpmp
    ${BLAKE3_LIBRARY}     # Add blake3
    ${ZLIB_LIBRARY}     # Add zlib
)

# Executables
add_executable(quids src/main.cpp)
target_link_libraries(quids PRIVATE quids_core)

# Tests
#enable_testing()
#add_executable(enhanced_ml_tests tests/evm/EVMExecutorTest.cpp tests/evm/uint256Test.cpp)
#target_link_libraries(enhanced_ml_tests PRIVATE quids_core GTest::GTest)

# Installation
install(TARGETS quids_core quids_network quids_blockchain quids_evm quids_common
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)



cmake_minimum_required(VERSION 3.16)

# Enable testing
enable_testing()

# Find required packages
find_package(GTest REQUIRED)
find_package(OpenMP REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenSSL 3.0 REQUIRED)

# Add OQS provider source
set(OQS_PROVIDER_DIR "/Users/nick/Desktop/Desktop - nick's iMac Pro - 1/whatev/oqs-provider")
if(NOT EXISTS ${OQS_PROVIDER_DIR})
    message(FATAL_ERROR "OQS provider directory not found at ${OQS_PROVIDER_DIR}")
endif()

include_directories(${OQS_PROVIDER_DIR}/include)
add_subdirectory(${OQS_PROVIDER_DIR} oqs_provider_build)

# Collect all test source files
file(GLOB_RECURSE TEST_SOURCES
    "rollup/*.cpp"
    "quantum/*.cpp"
    "zkp/*.cpp"
    "evm/*.cpp"
  
    "network/*.cpp"
    "blockchain/*.cpp"
    "consensus/*.cpp"
)

# Create test executable
add_executable(enhanced_ml_tests ${TEST_SOURCES})

target_link_libraries(enhanced_ml_tests
    PRIVATE
    qzkp_core
    GTest::GTest
    GTest::Main
    fmt::fmt
    OpenMP::OpenMP_CXX
    OpenSSL::SSL
    OpenSSL::Crypto
    oqsprovider
)

# Add test
add_test(
    NAME enhanced_ml_tests
    COMMAND enhanced_ml_tests
)

# Add benchmark-specific test
add_test(
    NAME rollup_benchmarks
    COMMAND enhanced_ml_tests --gtest_filter="RollupBenchmarkTest.*"
)

# Add separate high performance benchmark target
add_executable(rollup_benchmarks
    rollup/RollupBenchmarkTests.cpp
)

target_link_libraries(rollup_benchmarks
    PRIVATE
    qzkp_core
    GTest::GTest
    GTest::Main
    fmt::fmt
    OpenMP::OpenMP_CXX
    OpenSSL::SSL
    OpenSSL::Crypto
    oqsprovider
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

# Add OQS provider configuration
add_definitions(-DOQS_PROVIDER_ENABLED)
if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl@3")
    
    # Set RPATH for OQS provider on macOS
    set_target_properties(rollup_benchmarks PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@loader_path;@loader_path/../lib"
    )
endif() 
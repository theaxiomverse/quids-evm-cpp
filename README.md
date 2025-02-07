# Quantum-Safe EVM (QUIDS)

[![Build Status](https://github.com/yourorg/qzkp-evm/actions/workflows/ci.yml/badge.svg)](https://github.com/yourorg/qzkp-evm/actions)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![Python Version](https://img.shields.io/badge/Python-3.9%2B-blue.svg)](https://www.python.org/)

A quantum-resistant blockchain with AI optimization, zero-knowledge proofs, and high-performance consensus.

## Documentation

### Core Process Flows
- [Main Process Flow](docs/process.md) - Overview of the entire system
- [Startup Flow](docs/startup_flow.md) - Node initialization and boot process
- [Post-Initialization Flow](docs/post_init_flow.md) - Post-boot operations
- [Transaction Flow](docs/transaction_flow.md) - Transaction processing pipeline
- [Block Flow](docs/block_flow.md) - Block creation and validation
- [AI Flow](docs/ai_flow.md) - AI system operations
- [EVM Flow](docs/evm_flow.md) - Smart contract execution
- [Upgrade Flow](docs/upgrade_flow.md) - System upgrade process

## Key Features

- **Quantum-Safe Cryptography**: Dilithium5, Falcon512, and SPHINCS+ with BLAKE3
- **High Performance**: ~3M TPS with batch processing
- **Advanced Consensus**: Proof of Batch Probabilistic Consensus (POBPC)
- **Zero-Knowledge Proofs**: Quantum measurement-based verification
- **Smart Contracts**: Extended Solidity with quantum-safe primitives
- **Autonomous Operation**: RL-based decision making and optimization
- **Dynamic Chain Structure**: Hierarchical chain management with auto-scaling
- **Real-time Optimization**: Continuous parameter tuning and adaptation

## Building

### Prerequisites
- CMake 3.20+
- Clang 15+ or GCC 12+
- Python 3.9+
- ZSTD 1.5+
- spdlog
- OpenSSL

### Build Instructions
```bash
# Clone the repository
git clone --recursive https://github.com/yourorg/qzkp-evm
cd qzkp-evm

# Create build directory
mkdir build && cd build
cmake ..
make
```

### Running
```bash
# Start node
./quids start --config=/path/to/config.json

# Start with specific options
./quids start --port=8545 --rpc-port=8546 --data-dir=/path/to/data

# Check status
./quids status

# Stop node
./quids stop
```

## Project Structure

```
.
├── docs/                   # Documentation
├── include/               
│   ├── cli/               # Command line interface
│   ├── control/           # Node control interface
│   └── node/              # Core node implementation
├── src/
│   ├── cli/               # CLI implementation
│   ├── control/           # Control implementation
│   └── node/              # Node implementation
├── test/                  # Test files
├── CMakeLists.txt         # Build configuration
└── README.md              # This file
```

## System Requirements

### Minimum
- CPU: 4 cores
- RAM: 8GB
- Storage: 100GB SSD
- Network: 100Mbps

### Recommended
- CPU: 16+ cores
- RAM: 32GB
- Storage: 1TB NVMe SSD
- Network: 1Gbps

## Implementation Status

See our [Implementation Status](docs/IMPLEMENTATION_STATUS.md) for current progress and roadmap.

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

See [Contributing Guidelines](CONTRIBUTING.md) for more details.

## Security

- Post-quantum cryptographic primitives
- Quantum-safe key generation
- Dual-layer storage with integrity checks
- Regular security audits

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Contact

- GitHub Issues: For bug reports and feature requests
- Discord: Join our community server
- Email: Contact the development team


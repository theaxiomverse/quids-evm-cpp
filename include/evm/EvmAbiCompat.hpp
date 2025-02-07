#pragma once

#include <string>
#include <vector>
#include <memory>
#include "evm/FloatingPoint.hpp"

namespace evm {

class EvmAbiCompat {
public:
    // ABI encoding types
    enum class AbiType {
        UINT,
        INT,
        ADDRESS,
        BOOL,
        STRING,
        BYTES,
        ARRAY,
        TUPLE
    };

    struct AbiParameter {
        AbiType type;
        std::string name;
        bool indexed;
        uint16_t size;  // For uint/int/bytes
        std::vector<AbiParameter> components;  // For tuple/array
    };

    // Constructor
    EvmAbiCompat();
    ~EvmAbiCompat();

    // Original EVM ABI encoding
    std::vector<uint8_t> originalEvmEncode(const std::vector<AbiParameter>& params,
                                         const std::vector<std::vector<uint8_t>>& values);
    
    std::vector<std::vector<uint8_t>> originalEvmDecode(
        const std::vector<AbiParameter>& params,
        const std::vector<uint8_t>& encoded_data);

    // Function signature helpers
    static std::string getFunctionSelector(const std::string& function_signature);
    static std::vector<uint8_t> encodeFunctionCall(
        const std::string& function_signature,
        const std::vector<std::vector<uint8_t>>& parameters);

    // Event encoding/decoding
    std::vector<uint8_t> encodeEvent(
        const std::string& event_name,
        const std::vector<AbiParameter>& params,
        const std::vector<std::vector<uint8_t>>& values);

    // Cross-chain message formatting
    struct CrossChainMessage {
        uint64_t source_chain_id;
        uint64_t destination_chain_id;
        std::vector<uint8_t> payload;
        std::vector<uint8_t> signature;
    };

    std::vector<uint8_t> encodeCrossChainMessage(const CrossChainMessage& message);
    CrossChainMessage decodeCrossChainMessage(const std::vector<uint8_t>& encoded);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Internal encoding helpers
    std::vector<uint8_t> encodeSingle(const AbiParameter& param,
                                    const std::vector<uint8_t>& value);
    std::vector<uint8_t> encodeArray(const AbiParameter& param,
                                   const std::vector<std::vector<uint8_t>>& values);
    std::vector<uint8_t> encodeTuple(const std::vector<AbiParameter>& params,
                                   const std::vector<std::vector<uint8_t>>& values);
};

} // namespace evm 
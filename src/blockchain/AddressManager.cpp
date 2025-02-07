#include "blockchain/AddressManager.hpp"
#include <blake3.h>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include "zkp/QZKPGenerator.hpp"
#include "quantum/QuantumState.hpp"
#include <random>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace quids {
namespace blockchain {

std::vector<uint8_t> AddressManager::LocationData::serialize() const {
    nlohmann::json j;
    j["latitude"] = latitude;
    j["longitude"] = longitude;
    j["country"] = country;
    j["city"] = city;
    std::string json_str = j.dump();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::optional<AddressManager::LocationData> AddressManager::LocationData::deserialize(
    const std::vector<uint8_t>& data
) {
    try {
        std::string json_str(data.begin(), data.end());
        auto j = nlohmann::json::parse(json_str);
        LocationData location;
        location.latitude = j["latitude"].get<double>();
        location.longitude = j["longitude"].get<double>();
        location.country = j["country"].get<std::string>();
        location.city = j["city"].get<std::string>();
        return location;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> AddressManager::generateAddress(
    const LocationData& location,
    const std::string& purpose
) {
    try {
        // 1. Compute location hash
        auto location_hash = computeLocationHash(location);
        
        // 2. Create location vector for ZKP
        auto location_vector = createLocationVector(location);
        
        // 3. Generate ZKP commitment and proof
        auto commitment = generateZKPCommitment(location_vector, location_hash);
        auto proof = generateZKPProof(location_vector, commitment);
        
        // 4. Combine components and encode
        AddressComponents components{
            location_hash,
            commitment,
            proof,
            purpose
        };
        
        return encodeAddress(components);
    } catch (...) {
        return std::nullopt;
    }
}

std::array<uint8_t, 32> AddressManager::computeLocationHash(const LocationData& location) {
    std::array<uint8_t, 32> hash;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    
    auto serialized = location.serialize();
    blake3_hasher_update(&hasher, serialized.data(), serialized.size());
    blake3_hasher_finalize(&hasher, hash.data(), hash.size());
    
    return hash;
}

std::vector<double> AddressManager::createLocationVector(const LocationData& location) {
    std::vector<double> vec(LOCATION_VECTOR_SIZE, 0.0);
    vec[0] = location.latitude;
    vec[1] = location.longitude;
    // Remaining dimensions are zero-padded for enhanced security
    return vec;
}

std::vector<uint8_t> AddressManager::generateZKPCommitment(
    const std::vector<double>& location_vector,
    [[maybe_unused]] const std::array<uint8_t, 32>& identifier
) {
    // Convert location vector to quantum state
    quantum::QuantumState state(LOCATION_VECTOR_SIZE);
    for (size_t i = 0; i < location_vector.size(); i++) {
        state.setAmplitude(i, std::complex<double>(location_vector[i], 0.0));
    }
    
    // Generate QZKP proof
    zkp::QZKPGenerator qzkp;
    auto proof = qzkp.generate_proof(state);
    
    // Convert commitment to bytes
    std::vector<uint8_t> commitment_bytes;
    commitment_bytes.reserve(proof.commitment.size() * sizeof(std::complex<double>));
    
    for (const auto& amp : proof.commitment) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&amp);
        commitment_bytes.insert(commitment_bytes.end(), bytes, bytes + sizeof(std::complex<double>));
    }
    
    return commitment_bytes;
}

std::vector<uint8_t> AddressManager::generateZKPProof(
    const std::vector<double>& location_vector,
    const std::vector<uint8_t>& commitment
) {
    // Convert location vector to quantum state
    quantum::QuantumState state(LOCATION_VECTOR_SIZE);
    for (size_t i = 0; i < location_vector.size(); i++) {
        state.setAmplitude(i, std::complex<double>(location_vector[i], 0.0));
    }
    
    // Generate QZKP proof
    zkp::QZKPGenerator qzkp;
    auto proof = qzkp.generate_proof(state);
    
    return proof.proof_data;
}

std::string AddressManager::encodeAddress(const AddressComponents& components) {
    // Combine all components into a single byte vector
    std::vector<uint8_t> combined;
    
    // Add location hash
    combined.insert(combined.end(), 
        components.location_hash.begin(), 
        components.location_hash.end());
    
    // Add commitment size and data
    uint32_t commitment_size = components.zkp_commitment.size();
    const uint8_t* size_bytes = reinterpret_cast<const uint8_t*>(&commitment_size);
    combined.insert(combined.end(), size_bytes, size_bytes + sizeof(uint32_t));
    combined.insert(combined.end(),
        components.zkp_commitment.begin(),
        components.zkp_commitment.end());
    
    // Add proof size and data
    uint32_t proof_size = components.zkp_proof.size();
    size_bytes = reinterpret_cast<const uint8_t*>(&proof_size);
    combined.insert(combined.end(), size_bytes, size_bytes + sizeof(uint32_t));
    combined.insert(combined.end(),
        components.zkp_proof.begin(),
        components.zkp_proof.end());
    
    // Add purpose string
    combined.insert(combined.end(),
        components.purpose.begin(),
        components.purpose.end());
    
    // Hash the combined data
    std::array<uint8_t, 32> final_hash;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, combined.data(), combined.size());
    blake3_hasher_finalize(&hasher, final_hash.data(), final_hash.size());
    
    // Convert to hex string with prefix
    std::stringstream ss;
    ss << ADDRESS_PREFIX;
    for (uint8_t byte : final_hash) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    
    return ss.str();
}

std::optional<AddressManager::AddressComponents> AddressManager::decodeAddress(
    const std::string& address
) {
    try {
        // Verify prefix
        if (address.substr(0, strlen(ADDRESS_PREFIX)) != ADDRESS_PREFIX) {
            return std::nullopt;
        }
        
        // Convert hex string to bytes
        std::string hex = address.substr(strlen(ADDRESS_PREFIX));
        if (hex.length() != 64) {  // 32 bytes * 2 chars per byte
            return std::nullopt;
        }
        
        std::array<uint8_t, 32> hash;
        for (size_t i = 0; i < 32; i++) {
            std::string byte_str = hex.substr(i * 2, 2);
            hash[i] = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        }
        
        // For now, return minimal components
        // In a real implementation, you'd need to store and retrieve the full components
        return AddressComponents{
            hash,
            std::vector<uint8_t>{},  // commitment
            std::vector<uint8_t>{},  // proof
            "EOA"  // default purpose
        };
        
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<Account> AddressManager::createAccount(
    const std::string& address,
    uint64_t initial_balance
) {
    // First verify the address is valid
    if (!verifyAddress(address)) {
        return std::nullopt;
    }

    // Decode address components
    auto components = decodeAddress(address);
    if (!components) {
        return std::nullopt;
    }

    // Create appropriate account type based on purpose
    if (components->purpose == "EOA") {
        return Account(address, initial_balance);
    } else if (components->purpose == "CONTRACT") {
        // For contract accounts, we need bytecode
        // For now, create empty contract
        std::vector<uint8_t> empty_code;
        return Account(address, empty_code, initial_balance);
    }

    return std::nullopt;
}

bool AddressManager::verifyAddress(const std::string& address) {
    // 1. Check prefix
    if (address.substr(0, strlen(ADDRESS_PREFIX)) != ADDRESS_PREFIX) {
        return false;
    }

    // 2. Check length (prefix + 64 hex chars)
    if (address.length() != strlen(ADDRESS_PREFIX) + 64) {
        return false;
    }

    // 3. Verify hex characters
    std::string hex = address.substr(strlen(ADDRESS_PREFIX));
    return std::all_of(hex.begin(), hex.end(), [](char c) {
        return std::isxdigit(static_cast<unsigned char>(c));
    });
}

bool AddressManager::verifyLocation(
    const std::string& address,
    const LocationData& location
) {
    // 1. Decode address
    auto components = decodeAddress(address);
    if (!components) {
        return false;
    }

    // 2. Compute location hash and verify
    auto computed_hash = computeLocationHash(location);
    if (computed_hash != components->location_hash) {
        return false;
    }

    // 3. Create location vector for verification
    auto location_vector = createLocationVector(location);

    // 4. Convert to quantum state for verification
    quantum::QuantumState state(LOCATION_VECTOR_SIZE);
    for (size_t i = 0; i < location_vector.size(); i++) {
        state.setAmplitude(i, std::complex<double>(location_vector[i], 0.0));
    }

    // 5. Verify ZKP
    zkp::QZKPGenerator qzkp;
    
    // Convert stored commitment back to quantum state format
    std::vector<std::complex<double>> commitment;
    commitment.reserve(components->zkp_commitment.size() / sizeof(double));
    
    for (size_t i = 0; i < components->zkp_commitment.size(); i += sizeof(double)) {
        double value;
        std::memcpy(&value, &components->zkp_commitment[i], sizeof(double));
        commitment.push_back(std::complex<double>(value, 0.0));
    }

    // Convert stored proof back to quantum state format
    std::vector<std::complex<double>> proof_data;
    proof_data.reserve(components->zkp_proof.size() / sizeof(double));
    
    for (size_t i = 0; i < components->zkp_proof.size(); i += sizeof(double)) {
        double value;
        std::memcpy(&value, &components->zkp_proof[i], sizeof(double));
        proof_data.push_back(std::complex<double>(value, 0.0));
    }

    // Create proof object for verification
    zkp::QZKPGenerator::Proof stored_proof{
        commitment,
        proof_data
    };

    // Verify the proof
    return qzkp.verify_proof(state, stored_proof);
}

AddressManager::VSSScheme AddressManager::generateShares(
    const LocationData& location,
    size_t num_shares,
    size_t threshold
) {
    try {
        if (num_shares < MIN_SHARES || num_shares > MAX_SHARES || threshold > num_shares) {
            spdlog::error("Invalid VSS parameters: shares={}, threshold={}", num_shares, threshold);
            throw std::invalid_argument("Invalid share parameters");
        }

        // Convert location to quantum state vector
        auto location_vector = createLocationVector(location);
        if (location_vector.size() != LOCATION_VECTOR_SIZE) {
            spdlog::error("Invalid location vector size: {}", location_vector.size());
            throw std::runtime_error("Invalid location vector");
        }

        quantum::QuantumState state(LOCATION_VECTOR_SIZE);
        try {
            for (size_t i = 0; i < location_vector.size(); i++) {
                state.setAmplitude(i, std::complex<double>(location_vector[i], 0.0));
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to set quantum state: {}", e.what());
            throw;
        }

        // Create polynomial coefficients
        std::vector<double> coefficients(POLYNOMIAL_DEGREE);
        coefficients[0] = location_vector[0]; // x-coordinate
        coefficients[1] = location_vector[1]; // y-coordinate
        
        // Generate random coefficients for higher degrees
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_real_distribution<double> dist(-1.0, 1.0);
        
        for (size_t i = 2; i < POLYNOMIAL_DEGREE; i++) {
            coefficients[i] = dist(gen);
        }

        // Create polynomial
        auto polynomial = createPolynomial(coefficients, threshold - 1);

        // Generate shares
        VSSScheme scheme;
        scheme.threshold = threshold;
        scheme.shares.reserve(num_shares);

        for (size_t i = 1; i <= num_shares; i++) {
            std::complex<double> x(static_cast<double>(i), 0.0);
            auto share_data = evaluatePolynomial(polynomial, x);
            
            VSSShare share;
            share.data = share_data;
            share.index = i;
            share.commitment = computeShareCommitment(share_data);
            
            scheme.shares.push_back(share);
        }

        // Compute root commitment
        scheme.root_commitment = computeShareCommitment(polynomial);

        spdlog::debug("Generated {} shares with threshold {}", num_shares, threshold);
        return scheme;
    } catch (const std::exception& e) {
        spdlog::error("Failed to generate shares: {}", e.what());
        throw;
    }
}

bool AddressManager::verifyShare(
    const VSSShare& share,
    const std::array<uint8_t, 32>& root_commitment
) {
    try {
        // Validate share data
        if (share.data.empty() || share.data.size() != LOCATION_VECTOR_SIZE) {
            spdlog::error("Invalid share data size: {}", share.data.size());
            return false;
        }

        if (share.index == 0) {
            spdlog::error("Invalid share index: 0");
            return false;
        }

        // Verify share commitment
        auto computed_commitment = computeShareCommitment(share.data);
        if (computed_commitment != share.commitment) {
            spdlog::warn("Share commitment verification failed");
            return false;
        }

        // Verify against root commitment using quantum state verification
        quantum::QuantumState share_state(share.data.size());
        for (size_t i = 0; i < share.data.size(); i++) {
            share_state.setAmplitude(i, share.data[i]);
        }

        // Use QZKP to verify the share against root commitment
        zkp::QZKPGenerator qzkp;
        return qzkp.verify_share(share_state, root_commitment);

        spdlog::debug("Share {} verified successfully", share.index);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Share verification failed: {}", e.what());
        return false;
    }
}

std::optional<AddressManager::LocationData> AddressManager::reconstructLocation(
    const std::vector<VSSShare>& shares,
    size_t threshold
) {
    try {
        if (shares.empty()) {
            spdlog::error("Empty shares vector");
            return std::nullopt;
        }

        if (shares.size() < threshold) {
            spdlog::error("Insufficient shares: {} < {}", shares.size(), threshold);
            return std::nullopt;
        }

        // Validate share data consistency
        for (const auto& share : shares) {
            if (share.data.size() != LOCATION_VECTOR_SIZE) {
                spdlog::error("Invalid share data size: {}", share.data.size());
                return std::nullopt;
            }
        }

        // Use Lagrange interpolation to reconstruct the secret
        std::vector<std::complex<double>> reconstructed(LOCATION_VECTOR_SIZE);
        
        for (size_t i = 0; i < threshold; i++) {
            std::complex<double> lagrange_basis(1.0, 0.0);
            
            for (size_t j = 0; j < threshold; j++) {
                if (i != j) {
                    std::complex<double> numerator(-static_cast<double>(shares[j].index), 0.0);
                    std::complex<double> denominator(
                        static_cast<double>(shares[i].index - shares[j].index), 0.0
                    );
                    lagrange_basis *= numerator / denominator;
                }
            }

            for (size_t k = 0; k < LOCATION_VECTOR_SIZE; k++) {
                reconstructed[k] += shares[i].data[k] * lagrange_basis;
            }
        }

        // Convert reconstructed values back to LocationData
        LocationData location;
        location.latitude = std::real(reconstructed[0]);
        location.longitude = std::real(reconstructed[1]);
        
        // Verify reconstruction using quantum state comparison
        quantum::QuantumState reconstructed_state(LOCATION_VECTOR_SIZE);
        for (size_t i = 0; i < reconstructed.size(); i++) {
            reconstructed_state.setAmplitude(i, reconstructed[i]);
        }

        if (!reconstructed_state.isValid()) {
            return std::nullopt;
        }

        // Additional validation of reconstructed location
        if (std::isnan(location.latitude) || std::isnan(location.longitude) ||
            std::abs(location.latitude) > 90.0 || std::abs(location.longitude) > 180.0) {
            spdlog::error("Invalid reconstructed coordinates: lat={}, lon={}", 
                location.latitude, location.longitude);
            return std::nullopt;
        }

        spdlog::debug("Location reconstructed successfully from {} shares", shares.size());
        return location;
    } catch (const std::exception& e) {
        spdlog::error("Location reconstruction failed: {}", e.what());
        return std::nullopt;
    }
}

// Helper methods implementation
std::vector<std::complex<double>> AddressManager::createPolynomial(
    const std::vector<double>& coefficients,
    size_t degree
) {
    std::vector<std::complex<double>> poly(degree + 1);
    for (size_t i = 0; i <= degree && i < coefficients.size(); i++) {
        poly[i] = std::complex<double>(coefficients[i], 0.0);
    }
    return poly;
}

std::vector<std::complex<double>> AddressManager::evaluatePolynomial(
    const std::vector<std::complex<double>>& poly,
    std::complex<double> x
) {
    std::vector<std::complex<double>> result(LOCATION_VECTOR_SIZE);
    std::complex<double> x_power(1.0, 0.0);
    
    for (const auto& coeff : poly) {
        for (size_t i = 0; i < LOCATION_VECTOR_SIZE; i++) {
            result[i] += coeff * x_power;
        }
        x_power *= x;
    }
    
    return result;
}

std::array<uint8_t, 32> AddressManager::computeShareCommitment(
    const std::vector<std::complex<double>>& share_data
) {
    std::array<uint8_t, 32> commitment;
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    // Hash real and imaginary parts of each complex number
    for (const auto& value : share_data) {
        double real_part = std::real(value);
        double imag_part = std::imag(value);
        blake3_hasher_update(&hasher, &real_part, sizeof(double));
        blake3_hasher_update(&hasher, &imag_part, sizeof(double));
    }

    blake3_hasher_finalize(&hasher, commitment.data(), commitment.size());
    return commitment;
}

// Convert complex vector to bytes
std::vector<uint8_t> serialize_complex_vector(const std::vector<std::complex<double>>& vec) {
    std::vector<uint8_t> bytes;
    bytes.reserve(vec.size() * sizeof(std::complex<double>));
    
    for (const auto& c : vec) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(&c);
        bytes.insert(bytes.end(), data, data + sizeof(std::complex<double>));
    }
    
    return bytes;
}

// In verify_proof method
bool AddressManager::verify_proof(const quantum::QuantumState& state, const QZKPGenerator::Proof& stored_proof) {
    return qzkp.verify_proof(stored_proof, state);
}

} // namespace blockchain
} // namespace quids 
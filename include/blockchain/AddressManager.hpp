#pragma once

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <complex>
#include "blockchain/Account.hpp"
#include "quantum/QuantumCircuit.hpp"
#include "zkp/QZKPGenerator.hpp"
#include "quantum/QuantumState.hpp"

namespace quids {
namespace blockchain {

class AddressManager {
public:
    struct LocationData {
        double latitude;
        double longitude;
        std::string country;
        std::string city;
        
        std::vector<uint8_t> serialize() const;
        static std::optional<LocationData> deserialize(const std::vector<uint8_t>& data);
    };

    struct AddressComponents {
        std::array<uint8_t, 32> location_hash;
        std::vector<uint8_t> zkp_commitment;
        std::vector<uint8_t> zkp_proof;
        std::string purpose;
    };

    struct VSSShare {
        std::vector<std::complex<double>> data;
        size_t index;
        std::array<uint8_t, 32> commitment;
    };

    struct VSSScheme {
        std::vector<VSSShare> shares;
        size_t threshold;
        std::array<uint8_t, 32> root_commitment;
    };

    // Generate new address with location verification
    static std::optional<std::string> generateAddress(
        const LocationData& location,
        const std::string& purpose = "EOA"
    );

    // Create account from verified address
    static std::optional<Account> createAccount(
        const std::string& address,
        uint64_t initial_balance = 0
    );

    // Verify address components
    static bool verifyAddress(const std::string& address);
    static bool verifyLocation(
        const std::string& address,
        const LocationData& location
    );

    // VSS methods
    static VSSScheme generateShares(
        const LocationData& location,
        size_t num_shares,
        size_t threshold
    );

    static bool verifyShare(
        const VSSShare& share,
        const std::array<uint8_t, 32>& root_commitment
    );

    static std::optional<LocationData> reconstructLocation(
        const std::vector<VSSShare>& shares,
        size_t threshold
    );

private:
    // Internal helper methods
    static std::array<uint8_t, 32> computeLocationHash(const LocationData& location);
    static std::vector<double> createLocationVector(const LocationData& location);
    
    static std::vector<uint8_t> generateZKPCommitment(
        const std::vector<double>& location_vector,
        const std::array<uint8_t, 32>& identifier
    );
    
    static std::vector<uint8_t> generateZKPProof(
        const std::vector<double>& location_vector,
        const std::vector<uint8_t>& commitment
    );

    static std::string encodeAddress(const AddressComponents& components);
    static std::optional<AddressComponents> decodeAddress(const std::string& address);

    // Constants
    static constexpr size_t LOCATION_VECTOR_SIZE = 8;
    static constexpr char ADDRESS_PREFIX[] = "AXM_";

    // VSS helper methods
    static std::vector<std::complex<double>> createPolynomial(
        const std::vector<double>& coefficients,
        size_t degree
    );

    static std::vector<std::complex<double>> evaluatePolynomial(
        const std::vector<std::complex<double>>& poly,
        std::complex<double> x
    );

    static std::array<uint8_t, 32> computeShareCommitment(
        const std::vector<std::complex<double>>& share_data
    );

    // Constants for VSS
    static constexpr size_t MIN_SHARES = 3;
    static constexpr size_t MAX_SHARES = 10;
    static constexpr size_t POLYNOMIAL_DEGREE = 8;
};

} // namespace blockchain
} // namespace quids 
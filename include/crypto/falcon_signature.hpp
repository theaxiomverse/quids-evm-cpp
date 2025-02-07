#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <type_traits>

#include <falcon/falcon.hpp>



namespace quids {
namespace crypto {

namespace falcon {
class FalconSignature {
public:

    auto generate_key_pair() -> std::pair<std::string, std::string>;
    auto sign_message(const std::string& message) -> std::string;
    auto verify_signature(const std::string& message, const std::string& signature) -> bool;
    auto import_key_pair(const std::string& public_key, const std::string& secret_key) -> bool;
    auto export_key_pair() -> std::pair<std::string, std::string>;

    size_t N = 512;

   size_t pklen;
   size_t sklen;
   size_t msglen = 32;

  uint8_t* public_key;
  uint8_t* secret_key;
  uint8_t* signature;
  uint8_t* message;

    FalconSignature(size_t N_);
    ~FalconSignature();
};

} // namespace falcon
} // namespace crypto
} // namespace quids

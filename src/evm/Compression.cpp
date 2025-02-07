#include "evm/Compression.hpp"
#include <zlib.h>
#include <stdexcept>

namespace evm {

struct Compression::Impl {
    uint8_t level{6};  // Default compression level
    
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data) const {
        if (data.empty()) return {};
        
        uLongf compressed_size = compressBound(data.size());
        std::vector<uint8_t> compressed_data(compressed_size);
        
        int result = compress2(compressed_data.data(), &compressed_size,
                              data.data(), data.size(),
                              Z_BEST_COMPRESSION);
        
        if (result != Z_OK) {
            throw std::runtime_error("Compression failed");
        }
        
        compressed_data.resize(compressed_size);
        return compressed_data;
    }
    
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data) const {
        if (compressed_data.empty()) return {};
        
        // Start with a reasonable buffer size
        uLongf decompressed_size = compressed_data.size() * 4;
        std::vector<uint8_t> decompressed_data(decompressed_size);
        
        while (true) {
            int result = uncompress(decompressed_data.data(), &decompressed_size,
                                  compressed_data.data(), compressed_data.size());
            
            if (result == Z_OK) {
                decompressed_data.resize(decompressed_size);
                return decompressed_data;
            }
            
            if (result != Z_BUF_ERROR) {
                throw std::runtime_error("Decompression failed");
            }
            
            // Buffer was too small, try with a larger size
            decompressed_size *= 2;
            decompressed_data.resize(decompressed_size);
        }
    }
};

Compression::Compression() : impl_(std::make_unique<Impl>()) {}
Compression::~Compression() = default;

Compression::Compression(Compression&&) noexcept = default;
Compression& Compression::operator=(Compression&&) noexcept = default;

std::vector<uint8_t> Compression::compress(const std::vector<uint8_t>& data) const {
    return impl_->compress(data);
}

std::vector<uint8_t> Compression::decompress(const std::vector<uint8_t>& compressed_data) const {
    return impl_->decompress(compressed_data);
}

void Compression::set_level(uint8_t level) {
    impl_->level = level;
}

uint8_t Compression::get_level() const {
    return impl_->level;
}

} // namespace evm 
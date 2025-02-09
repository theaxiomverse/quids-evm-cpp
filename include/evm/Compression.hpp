#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <string>

namespace evm {

class Compression {
public:
    // Constructors and destructor
    Compression();
    ~Compression();

    // Disable copy
    Compression(const Compression&) = delete;
    Compression& operator=(const Compression&) = delete;

    // Enable move
    Compression(Compression&&) noexcept;
    Compression& operator=(Compression&&) noexcept;

    enum class Algorithm {
        ZLIB,
        SNAPPY,
        LZ4,
        ZSTD
    };
    
    explicit Compression(Algorithm algo = Algorithm::ZSTD);
    
    // Compression operations
    [[nodiscard]] std::vector<uint8_t> compress(const std::vector<uint8_t>& data) const;
    [[nodiscard]] std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed_data) const;
    
    // Configuration
    void set_algorithm(Algorithm algo);
    void set_level(uint8_t level);
    [[nodiscard]] uint8_t get_level() const;
    
    // Statistics
    [[nodiscard]] double get_compression_ratio() const;
    [[nodiscard]] size_t get_total_compressed_size() const;
    [[nodiscard]] size_t get_total_original_size() const;
    
    // Dictionary-based compression
    void train_dictionary(const std::vector<std::vector<uint8_t>>& samples);
    void set_dictionary(const std::vector<uint8_t>& dictionary);
    [[nodiscard]] std::vector<uint8_t> get_dictionary() const;
    
    // Stream interface for large data
    class Stream {
    public:
        Stream() = default;
        ~Stream() = default;

        // Disable copy and move
        Stream(const Stream&) = delete;
        Stream& operator=(const Stream&) = delete;
        Stream(Stream&&) = delete;
        Stream& operator=(Stream&&) = delete;

        void write(const std::vector<uint8_t>& data);
        [[nodiscard]] std::vector<uint8_t> read(size_t size);
        void flush();
        
    private:
        std::vector<uint8_t> buffer_;
        size_t position_{0};
    };
    
    [[nodiscard]] std::unique_ptr<Stream> create_compression_stream();
    [[nodiscard]] std::unique_ptr<Stream> create_decompression_stream();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    Algorithm algorithm_;
    uint8_t compression_level_;
    std::vector<uint8_t> dictionary_;
    
    // Statistics
    size_t total_compressed_size_{0};
    size_t total_original_size_{0};
};

} // namespace evm 
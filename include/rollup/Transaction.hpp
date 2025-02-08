#pragma once

#include "blockchain/Transaction.hpp"
#include <cstdint>
#include <vector>
#include <string>

namespace quids {
namespace rollup {

class Transaction : public blockchain::Transaction {
public:
    Transaction() = default;
    Transaction(const Transaction& other) : blockchain::Transaction(other) {
        setAmount(other.getAmount());
        setSender(other.getSender());
        setRecipient(other.getRecipient());
    }

    Transaction& operator=(const Transaction& other) {
        if (this != &other) {
            blockchain::Transaction::operator=(other);
            setAmount(other.getAmount());
            setSender(other.getSender());
            setRecipient(other.getRecipient());
        }
        return *this;
    }

    static Transaction create(const std::string& sender_, const std::string& recipient_, uint64_t amount_) {
        Transaction tx;
        tx.setSender(sender_);
        tx.setRecipient(recipient_);
        tx.setAmount(amount_);
        return tx;
    }

    bool sign(const std::array<uint8_t, 32>& /*private_key*/) {
        // Implement signing logic here
        std::vector<uint8_t> sig(64, 1); // Dummy signature for now
        setSignature(sig);
        return true;
    }

    // Getters/setters
    uint64_t getNonce() const { return nonce_; }
    void setNonce(uint64_t nonce) { nonce_ = nonce; }
    
    uint64_t getAmount() const { return amount_; }
    void setAmount(uint64_t amount) { amount_ = amount; }
    
    const std::string& getSender() const { return sender_; }
    void setSender(const std::string& sender) { sender_ = sender; }
    
    const std::string& getRecipient() const { return recipient_; }
    void setRecipient(const std::string& recipient) { recipient_ = recipient; }
    
    const std::vector<uint8_t>& getSignature() const { return signature_; }
    void setSignature(const std::vector<uint8_t>& sig) { signature_ = sig; }

    bool isValid() const {
        // Vectorizable validation logic
        bool valid = true;
        valid = valid && (nonce_ >= 0);
        valid = valid && (amount_ > 0);
        valid = valid && (!sender_.empty());
        valid = valid && (!recipient_.empty());
        valid = valid && (signature_.size() >= 64);
        return valid;
    }

private:
    uint64_t nonce_{0};
    uint64_t amount_{0};
    std::string sender_;
    std::string recipient_;
    std::vector<uint8_t> signature_;
};

} // namespace rollup
} // namespace quids 
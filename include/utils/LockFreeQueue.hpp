#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <type_traits>

namespace quids {
namespace utils {

template<typename T>
class LockFreeQueue {
    static_assert(std::is_move_constructible_v<T>, "T must be move constructible");
    static_assert(std::is_move_assignable_v<T>, "T must be move assignable");

private:
    struct Node {
        std::unique_ptr<T> data;
        std::atomic<Node*> next{nullptr};
        
        Node() = default;
        explicit Node(T&& item) : data(std::make_unique<T>(std::move(item))) {}
    };

    alignas(64) std::atomic<Node*> head_{nullptr};
    alignas(64) std::atomic<Node*> tail_{nullptr};
    alignas(64) std::atomic<size_t> size_{0};

public:
    LockFreeQueue() {
        auto dummy = std::make_unique<Node>();
        tail_.store(dummy.get());
        head_.store(dummy.release());
    }

    ~LockFreeQueue() {
        while (auto node = head_.load()) {
            head_.store(node->next.load());
            delete node;
        }
    }

    // Disable copying
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    // Enable moving
    LockFreeQueue(LockFreeQueue&& other) noexcept {
        auto dummy = std::make_unique<Node>();
        tail_.store(dummy.get());
        head_.store(dummy.release());
        
        // Move contents
        auto other_head = other.head_.load();
        auto other_tail = other.tail_.load();
        if (other_head != other_tail) {
            head_.store(other_head);
            tail_.store(other_tail);
            size_.store(other.size_.load());
            
            // Reset other queue
            auto new_dummy = std::make_unique<Node>();
            other.tail_.store(new_dummy.get());
            other.head_.store(new_dummy.release());
            other.size_.store(0);
        }
    }

    LockFreeQueue& operator=(LockFreeQueue&& other) noexcept {
        if (this != &other) {
            // Clear current queue
            while (auto node = head_.load()) {
                head_.store(node->next.load());
                delete node;
            }
            
            // Move contents
            auto other_head = other.head_.load();
            auto other_tail = other.tail_.load();
            if (other_head != other_tail) {
                head_.store(other_head);
                tail_.store(other_tail);
                size_.store(other.size_.load());
                
                // Reset other queue
                auto new_dummy = std::make_unique<Node>();
                other.tail_.store(new_dummy.get());
                other.head_.store(new_dummy.release());
                other.size_.store(0);
            } else {
                // Initialize empty queue
                auto dummy = std::make_unique<Node>();
                tail_.store(dummy.get());
                head_.store(dummy.release());
                size_.store(0);
            }
        }
        return *this;
    }

    void push(T item) {
        auto new_node = std::make_unique<Node>(std::move(item));
        auto new_node_ptr = new_node.get();
        
        while (true) {
            auto tail = tail_.load();
            auto next = tail->next.load();
            
            if (tail == tail_.load()) {
                if (next == nullptr) {
                    if (tail->next.compare_exchange_weak(next, new_node_ptr)) {
                        tail_.compare_exchange_strong(tail, new_node_ptr);
                        new_node.release();
                        size_.fetch_add(1);
                        return;
                    }
                } else {
                    tail_.compare_exchange_strong(tail, next);
                }
            }
        }
    }

    std::optional<T> pop() {
        while (true) {
            auto head = head_.load();
            auto tail = tail_.load();
            auto next = head->next.load();
            
            if (head == head_.load()) {
                if (head == tail) {
                    if (next == nullptr) {
                        return std::nullopt;
                    }
                    tail_.compare_exchange_strong(tail, next);
                } else {
                    if (next) {
                        auto result = std::move(*next->data);
                        if (head_.compare_exchange_weak(head, next)) {
                            size_.fetch_sub(1);
                            delete head;
                            return result;
                        }
                    }
                }
            }
        }
    }

    [[nodiscard]] size_t size() const noexcept {
        return size_.load();
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_.load() == 0;
    }

    void clear() {
        while (pop()) {}
    }
};

} // namespace utils
} // namespace quids 
#pragma once
#include <iostream>
#include <atomic>
#include <new>

#ifndef hardware_destructive_interference_size
#define hardware_destructive_interference_size 64
#endif

template <typename T, size_t Capacity>
class SPSCQueue {
    static_assert(std::atomic<size_t>::is_always_lock_free);
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of 2");

public:
    SPSCQueue() : head_(0), tail_(0) {
    }

    bool push(const T& value) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next_head = (head + 1) & (Capacity - 1);

        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; // Queue full
        }

        buffer_[head] = value;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    bool pop(T& value) {
        size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue empty
        }

        value = buffer_[tail];
        tail_.store((tail + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

private:
    alignas(hardware_destructive_interference_size) std::atomic<size_t> head_;
    char padding1_[hardware_destructive_interference_size - sizeof(size_t)]; // Padding to avoid false sharing
    alignas(hardware_destructive_interference_size) std::atomic<size_t> tail_;
    char padding2_[hardware_destructive_interference_size - sizeof(size_t)];
    alignas(hardware_destructive_interference_size) std::array<T, Capacity> buffer_;
};

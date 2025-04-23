#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <stdexcept>

#include "container/lock_free_queue_hazard.h"

void mpmc_test() {
    LockFreeQueue<int> q;
    constexpr int producer_count = 4;
    constexpr int consumer_count = 4;
    constexpr int items_per_producer = 10000;

    std::vector<std::thread> producers, consumers;
    std::atomic<int> total_dequeued{0};

    for (int i = 0; i < producer_count; ++i) {
        producers.emplace_back([&, i] {
            thread_local int count = 0;
            for (int j = 0; j < items_per_producer; ++j) {
                q.enqueue(i * items_per_producer + j);
                ++count;
            }
            std::cout << "Thread["  << i << "]" << count << std::endl;
        });
    }

    for (int i = 0; i < consumer_count; ++i) {
        consumers.emplace_back([&] {
            int val;
            while (total_dequeued.load() < producer_count * items_per_producer) {
                if (q.dequeue(val)) {
                    total_dequeued.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();

    std::cout << "Total dequeued: " << total_dequeued.load(std::memory_order_relaxed) << "\n";
}

int main() {
    mpmc_test();
    return 0;
}

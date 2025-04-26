#include <boost/lockfree/queue.hpp>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>
#include <chrono>

constexpr int NUM_PRODUCERS = 4;
constexpr int NUM_CONSUMERS = 4;
constexpr int ITEMS_PER_PRODUCER = 1000;

boost::lockfree::queue<int> queue(1024);  // capacity of queue

std::atomic<int> total_pushed{0};
std::atomic<int> total_popped{0};

void producer(int id) {
    for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
        int value = id * ITEMS_PER_PRODUCER + i;
        while (!queue.push(value)) {
            // Queue is full, retry
        }
        total_pushed.fetch_add(1, std::memory_order_relaxed);
    }
}

void consumer() {
    int value;
    while (total_popped.load(std::memory_order_relaxed) < NUM_PRODUCERS * ITEMS_PER_PRODUCER) {
        if (queue.pop(value)) {
            // std::cout << "Consumed: " << value << std::endl;
            total_popped.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

int main() {
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back(producer, i);
    }

    for (int i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back(consumer);
    }

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    std::cout << "Total pushed: " << total_pushed.load() << "\n";
    std::cout << "Total popped: " << total_popped.load() << "\n";
    std::cout << "Duration: " << duration.count() << " seconds\n";

    return 0;
}

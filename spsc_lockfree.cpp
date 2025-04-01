#include <atomic>
#include <array>
#include <iostream>
#include <thread>
#include <chrono>
#include <charconv>
#include <cstring>

#include "container/spsc_queue.h"

constexpr size_t QueueCapacity = 2048;
constexpr int TotalMessages = 1000;

template <typename T, size_t>
class SPSCQueue;

SPSCQueue<int, QueueCapacity> queue;

void producer() {
    for (int i = 0; i < TotalMessages; ++i) {
        while (!queue.push(i)) {
            std::this_thread::yield(); // Queue full, re-schedule and retry
        }
        std::cout << "Produced: " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    }
}

void consumer() {
    for (int i = 0; i < TotalMessages; ++i) {
        int value;
        while (!queue.pop(value)) {
            std::this_thread::yield(); // Queue empty, re-schedule and retry
        }
        std::cout << "Consumed: " << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate work
    }
}

int main() {
    auto start = std::chrono::high_resolution_clock::now(); // Start time

    std::thread prod(producer);
    std::thread cons(consumer);

    prod.join();
    cons.join();

    auto end = std::chrono::high_resolution_clock::now(); // End time
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Execution Time: " << elapsed.count() << " seconds" << std::endl;
    return 0;
}

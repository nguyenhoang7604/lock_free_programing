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

std::atomic<int> totalPushed{0};
std::atomic<int> totalPopped{0};

template <typename T, size_t>
class SPSCQueue;

SPSCQueue<int, QueueCapacity> queue;

void producer() {
    for (int i = 0; i < TotalMessages; ++i) {
        while (!queue.push(i)) {
            std::this_thread::yield(); // Queue full, re-schedule and retry
        }
        totalPushed.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void consumer() {
    for (int i = 0; i < TotalMessages; ++i) {
        int value;
        while (!queue.pop(value)) {
            std::this_thread::yield(); // Queue empty, re-schedule and retry
        }
        totalPopped.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
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

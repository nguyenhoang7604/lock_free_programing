#include <container/lock_free_queue_hazard.h>
#include <iostream>
#include <thread>

#include <gtest/gtest.h>

TEST(LockFreeQueue, writeReadSequentially) {
    constexpr int messages = 1024;
    constexpr int turns = 4;

    LockFreeQueue<int> queue;
    for (unsigned int turn = 0; turn < turns; turn++) {
        for (unsigned int write = 0; write < messages; write++) {
            int val = turn * messages + write;
            queue.enqueue(val);
        }

        for (unsigned int write = 0; write < messages; write++) {
            int value = 0;
            ASSERT_TRUE(queue.dequeue(value));
            ASSERT_EQ(turn * messages + write, value);
        }
    }

    ASSERT_EQ(queue.size(), 0);
}

TEST(LockFreeQueue, readEmptyQueue) {
    // Start a reader thread, confirm that reading empty queue fail gracefully
    LockFreeQueue<int> queue;

    auto reader = std::thread([&]() {
        int val = 0;
        EXPECT_FALSE(queue.dequeue(val));
        EXPECT_EQ(val, 0);
    });

    reader.join();
}

TEST(LockFreeQueue, concurrentWriters) {
    LockFreeQueue<int> queue;
    const int totalMessages = 100;
    const int numWriters = 10;

    std::atomic<int> messagesLeft = totalMessages;
    std::atomic<bool> writersDone{false};
    std::vector<int> result(totalMessages, 1);

    std::vector<std::thread> writers;
    for (int writer = 0; writer < numWriters; ++writer) {
        writers.emplace_back([&]() {
            while (true) {
                int value = messagesLeft.fetch_sub(1, std::memory_order_acq_rel);
                if (value < 0) {
                    break;
                }
                // std::cout << "Enqueue: " << value << std::endl;
                queue.enqueue(value);
            }
        });
    }

    std::thread reader([&]() {
        while (true) {
            int value = 0;
            if (queue.dequeue(value)) {
                // mark the value we read
                if (value >= 0 && value < static_cast<int>(result.size())) {
                    // std::cout << "Dequeue: " << value << std::endl;
                    result[value] = 0;
                }
            } else if (writersDone.load(std::memory_order_acquire)) {
                // writers are done and queue is empty
                if (!queue.size()) {
                    break;
                }
            }
            // else: queue was temporarily empty, retry
        }
    });

    for (auto& writer : writers) {
        writer.join();
    }
    writersDone.store(true, std::memory_order_release);
    reader.join();

    ASSERT_EQ(queue.size(), 0);
    for (auto value : result) {
        ASSERT_EQ(value, 0);
    }
}


TEST(LockFreeQueue, concurentReaders) {
    LockFreeQueue<int> queue;
    const int totalMessages = 100;
    const int numReaders = 10;

    std::atomic<int> messagesLeft = totalMessages;
    std::atomic<bool> writersDone{false};
    std::vector<int> result(totalMessages, 1);

    std::vector<std::thread> readers;
    for (int reader = 0; reader < numReaders; ++reader) {
        readers.emplace_back([&]() {
            while (true) {
                int value = 0;
                if (queue.dequeue(value)) {
                    // mark the value we read
                    if (value >= 0 && value < static_cast<int>(result.size())) {
                        // std::cout << "Dequeue: " << value << std::endl;
                        result[value] = 0;
                    }
                } else if (writersDone.load(std::memory_order_acquire)) {
                    // writers are done and queue is empty
                    if (!queue.size()) {
                        break;
                    }
                }
                // else: queue was temporarily empty, retry
            }
        });
    }

    std::thread writer([&]() {
            while (true) {
                int value = messagesLeft.fetch_sub(1, std::memory_order_acq_rel);
                if (value < 0) {
                    break;
                }
                // std::cout << "Enqueue: " << value << std::endl;
                queue.enqueue(value);
            }
    });


    writer.join();
    writersDone.store(true, std::memory_order_release);

    for (auto& reader : readers) {
        reader.join();
    }

    ASSERT_EQ(queue.size(), 0);
    for (auto value : result) {
        ASSERT_EQ(value, 0);
    }
}

TEST(LockFreeQueue, concurentWritersReaders) {
    LockFreeQueue<int> queue;
    constexpr int totalMessages = 100;
    constexpr int numReaders = 10;
    constexpr int numWriters = 10;

    std::atomic<int> messagesLeft = totalMessages;
    std::atomic<bool> writersDone{false};
    std::vector<int> result(totalMessages, 1);

    std::vector<std::thread> readers;
    for (int reader = 0; reader < numReaders; ++reader) {
        readers.emplace_back([&]() {
            while (true) {
                int value = 0;
                if (queue.dequeue(value)) {
                    // mark the value we read
                    if (value >= 0 && value < static_cast<int>(result.size())) {
                        // std::cout << "Dequeue: " << value << std::endl;
                        result[value] = 0;
                    }
                } else if (writersDone.load(std::memory_order_acquire)) {
                    // writers are done and queue is empty
                    if (!queue.size()) {
                        break;
                    }
                }
                // else: queue was temporarily empty, retry
            }
        });
    }

    std::vector<std::thread> writers;
    for (int writer = 0; writer < numWriters; ++writer) {
        writers.emplace_back([&]() {
            while (true) {
                int value = messagesLeft.fetch_sub(1, std::memory_order_acq_rel);
                if (value < 0) {
                    break;
                }
                // std::cout << "Enqueue: " << value << std::endl;
                queue.enqueue(value);
            }
        });
    }


    for (auto& writer : writers) {
        writer.join();
    }

    writersDone.store(true, std::memory_order_release);

    for (auto& reader : readers) {
        reader.join();
    }

    ASSERT_EQ(queue.size(), 0);
    for (auto value : result) {
        ASSERT_EQ(value, 0);
    }
}

/*
    Hazard pointer pool size is fixed by 100
    Will implement a new version where the pool size is flexible
*/
TEST(LockFreeQueue, runOutOfHazardPointer) {
    LockFreeQueue<int> queue;
    constexpr int totalMessages = 1000;
    constexpr int numReaders = 100;
    constexpr int numWriters = 100;

    std::atomic<int> messagesLeft = totalMessages;
    std::atomic<bool> writersDone{false};
    std::vector<int> result(totalMessages, 1);

    std::vector<std::thread> readers;
    for (int reader = 0; reader < numReaders; ++reader) {
        readers.emplace_back([&]() {
            while (true) {
                int value = 0;
                try {
                    if (queue.dequeue(value)) {
                        // mark the value we read
                        if (value >= 0 && value < static_cast<int>(result.size())) {
                            // std::cout << "Dequeue: " << value << std::endl;
                            result[value] = 0;
                        }
                    } else if (writersDone.load(std::memory_order_acquire)) {
                        // writers are done and queue is empty
                        if (!queue.size()) {
                            break;
                        }
                    }
                } catch (std::exception const& err) {
                    EXPECT_EQ(err.what(), std::string("No available hazard pointer\n"));
                    break;
                }
                // else: queue was temporarily empty, retry
            }
        });
    }

    std::vector<std::thread> writers;
    for (int writer = 0; writer < numWriters; ++writer) {
        writers.emplace_back([&]() {
            while (true) {
                int value = messagesLeft.fetch_sub(1, std::memory_order_acq_rel);
                if (value < 0) {
                    break;
                }

                try {
                    queue.enqueue(value);
                } catch (std::exception const& err) {
                    EXPECT_EQ(err.what(), std::string("No available hazard pointer\n"));
                    break;
                }
            }
        });
    }


    for (auto& writer : writers) {
        writer.join();
    }

    writersDone.store(true, std::memory_order_release);

    for (auto& reader : readers) {
        reader.join();
    }
}
#pragma once
#include <atomic>
#include "hazard_pointer.h"

template<typename T>
struct Node {
    static_assert(std::atomic<T*>::is_always_lock_free);

    std::atomic<T*> data_;
    std::atomic<Node<T>*> next_;
    Node() : data_(nullptr), next_(nullptr) {}
    Node(T* const data) : data_(data), next_(nullptr) {}

    ~Node() {
        auto ptr = data_.load(std::memory_order_acquire);
        if (data_.compare_exchange_strong(ptr, nullptr,
                                      std::memory_order_acq_rel,
                                      std::memory_order_relaxed)) {
            delete ptr;
        }
    }
};

template<typename T, Nodeable Node = Node<T>>
class LockFreeQueue {
public:
    LockFreeQueue() {
        auto dummy = new Node();
        head_.store(dummy);
        tail_.store(dummy);
    }

    ~LockFreeQueue() {
        while (Node* oldNode = head_.load())
        {
            head_.store(oldNode->next_);
            delete oldNode;
        }
    }

    static_assert(std::atomic<Node*>::is_always_lock_free);
    static_assert(std::atomic<size_t>::is_always_lock_free);

    void enqueue(T const& value);

    bool dequeue(T& result);

    size_t size() const { return size_.load(std::memory_order_relaxed); }

private:
    bool tryInsertNewTail(Node* oldTail, Node* newTail) {
        Node* nullNode = nullptr;
        if (oldTail->next_.compare_exchange_strong(nullNode, newTail,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed)) {
            tail_.store(newTail, std::memory_order_release);
            size_.fetch_add(1, std::memory_order_relaxed);
            return true;
        } else {
            return false;
        }
    }

private:
    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    std::atomic<size_t> size_;
    RetiredList<T> retiredList_;
};

template<typename T, Nodeable Node>
inline void LockFreeQueue<T, Node>::enqueue(T const& value) {
    std::atomic<Node*>& hazardPointer = getHazardPointer<T>();
    Node* newTail = new Node();
    T* data = new T(value);

    for (;;) {
        Node* oldTail = tail_.load(std::memory_order_acquire);
        Node* tmpNode;
        do {
            hazardPointer.store(nullptr);
            tmpNode = oldTail;
            hazardPointer.store(oldTail);
            oldTail = tail_.load(std::memory_order_acquire);
        } while (oldTail != tmpNode);

        T* expectedValue = nullptr;
        if (oldTail->data_.compare_exchange_strong(expectedValue, data,
                                                std::memory_order_release,
                                                std::memory_order_relaxed)) {
            if (!tryInsertNewTail(oldTail, newTail)) delete newTail;

            return;
        } else {
            if (tryInsertNewTail(oldTail, newTail)) {
                newTail = new Node();
            }
        }
    }
}

template <typename T, Nodeable Node>
bool LockFreeQueue<T,Node>::dequeue(T& result) {
    std::atomic<Node*>& hazardPointer = getHazardPointer<T>();
    Node* oldHead;

    for (;;) {
        oldHead = head_.load();

        if (tail_.load(std::memory_order_acquire) == oldHead) return false;

        hazardPointer.store(oldHead);

        if (head_.load() != oldHead) {
            hazardPointer.store(nullptr);
            continue;
        }

        Node* nextHead = oldHead->next_.load();

        if(head_.compare_exchange_strong(oldHead, nextHead,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
            auto data = oldHead->data_.load(std::memory_order_acquire);
            result = std::move(*data);
            break;
        }
    }

    hazardPointer.store(nullptr);
    size_.fetch_sub(1, std::memory_order_relaxed);

    if (isUsing(oldHead)) retiredList_.addNode(oldHead);
    else delete oldHead;

    retiredList_.deleteUnusedNodes();

    return true;
}

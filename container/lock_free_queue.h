#pragma once
#include <atomic>
#include <iostream>

struct Node {
    int value;
    std::atomic<Node*> next;
    Node(int val) : value(val), next(nullptr) {}
};


/*!
 * Simple lock free queue without guard (ABA problems, no delay deletion)
 */

class LockFreeQueue {
public:
    LockFreeQueue() {
        Node* dummy = new Node(-1);
        head.store(dummy);
        tail.store(dummy);
    }

    void enqueue(int value) {
        Node* newNode = new Node(value);
        Node* oldTail;
        do {
            oldTail = tail.load();
        } while (!tail.compare_exchange_weak(oldTail, newNode));
        oldTail->next.store(newNode);
    }

    int dequeue() {
        Node* oldHead;
        do {
            oldHead = head.load();
            if (oldHead->next.load() == nullptr) return -1;
        } while (!head.compare_exchange_weak(oldHead, oldHead->next.load()));
        int value = oldHead->next.load()->value;
        delete oldHead;
        return value;
    }

private:
    std::atomic<Node*> head;
    std::atomic<Node*> tail;
};

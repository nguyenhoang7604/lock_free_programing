#pragma once
#include <atomic>
#include <concepts>
#include <thread>

constexpr std::size_t MaxHazardPointers = 100;

template<typename T>
concept Nodeable = requires(T a) {
    { T::data_ };
    { *a.next_ } -> std::same_as<T&>;
};

template<typename T>
struct Node;

template <typename T, Nodeable Node = Node<T>>
struct HazardPointer
{
    std::atomic<std::thread::id> id_;
    std::atomic<Node*> pointer_;
};

template <typename T>
HazardPointer<T> HazardPointers[MaxHazardPointers];


template <typename T, Nodeable Node = Node<T>>
class HazardPointerOwner {

public:
    HazardPointerOwner(HazardPointerOwner const&) = delete;
    HazardPointerOwner& operator = (HazardPointerOwner const&) = delete;
    // void* operator new () = delete;
    // void operator delete(void*) = delete;

    HazardPointerOwner() : hazardPointer_(nullptr) {
        for (size_t i = 0; i < MaxHazardPointers; ++i) {
            std::thread::id oldId;
            if (HazardPointers<T>[i].id_.compare_exchange_strong(oldId, std::this_thread::get_id())) {
                hazardPointer_ = &HazardPointers<T>[i];
                return;
            }
        }
        if (nullptr == hazardPointer_)
            throw std::runtime_error("No available hazard pointer\n");
    }

    ~HazardPointerOwner() {
        hazardPointer_->pointer_.store(nullptr);
        hazardPointer_->id_.store(std::thread::id());
    }

    std::atomic<Node*>& get() {
        return hazardPointer_->pointer_;
    }

private:
    HazardPointer<T>* hazardPointer_;
};

template <typename T>
std::atomic<Node<T>*>& getHazardPointer()
{
    thread_local static HazardPointerOwner<T> pointer;
    return pointer.get();
}

template<typename T>
bool isUsing(Node<T>* node) {
    for (size_t i = 0; i < MaxHazardPointers; ++i) {
        if (HazardPointers<T>[i].pointer_.load() == node) return true;
    }
    return false;
}

template <typename T, Nodeable Node = Node<T>>
class RetiredList
{
private:
    struct RetiredNode {
        Node* node_;
        RetiredNode* next_;
        RetiredNode(Node* node) : node_(node), next_(nullptr) {}
        ~RetiredNode() {
            delete node_;
        }
    };

    void addToRetiredNodes(RetiredNode* node) {
        node->next_ = retiredNodes_.load();
        while(!retiredNodes_.compare_exchange_strong(node->next_, node,
                                                std::memory_order_release,
                                                std::memory_order_relaxed));
    }

public:
    RetiredList() : retiredNodes_ (nullptr) {}

    void addNode(Node* node) {
        addToRetiredNodes(new RetiredNode(node));
    }

    void deleteUnusedNodes() {
        RetiredNode* current = retiredNodes_.exchange(nullptr);
        while (nullptr != current) {
            RetiredNode* const next = current->next_;
            if (!isUsing(current->node_)) delete current;
            else addToRetiredNodes(current);
            current = next;
        }
    }

private:
    std::atomic<RetiredNode*> retiredNodes_;
};


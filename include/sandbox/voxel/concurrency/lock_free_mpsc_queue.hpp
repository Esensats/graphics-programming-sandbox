#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

namespace sandbox::voxel::concurrency {

template <typename T>
class LockFreeMpscQueue {
  public:
    LockFreeMpscQueue()
        : sentinel_(new Node())
        , head_(sentinel_)
        , tail_(sentinel_) {
    }

    LockFreeMpscQueue(const LockFreeMpscQueue&) = delete;
    LockFreeMpscQueue& operator=(const LockFreeMpscQueue&) = delete;

    ~LockFreeMpscQueue() {
        clear();
        delete tail_;
        tail_ = nullptr;
        sentinel_ = nullptr;
    }

    template <typename U>
    void push(U&& value) {
        static_assert(std::is_same_v<std::remove_cvref_t<U>, T>, "Queue value type mismatch");

        Node* node = new Node(std::forward<U>(value));
        Node* previous = head_.exchange(node, std::memory_order_acq_rel);
        previous->next.store(node, std::memory_order_release);
        size_.fetch_add(1, std::memory_order_release);
    }

    [[nodiscard]] bool try_pop(T& out) {
        Node* tail = tail_;
        Node* next = tail->next.load(std::memory_order_acquire);
        if (next == nullptr) {
            return false;
        }

        out = std::move(next->value());
        tail_ = next;
        delete tail;
        size_.fetch_sub(1, std::memory_order_release);
        return true;
    }

    void clear() {
        while (true) {
            Node* tail = tail_;
            Node* next = tail->next.load(std::memory_order_acquire);
            if (next == nullptr) {
                return;
            }

            tail_ = next;
            delete tail;
            size_.fetch_sub(1, std::memory_order_release);
        }
    }

    [[nodiscard]] std::size_t size() const {
        return size_.load(std::memory_order_acquire);
    }

  private:
    class Node {
      public:
        Node() = default;

        explicit Node(T&& value)
            : has_value_(true) {
            new (&storage_) T(std::move(value));
        }

        explicit Node(const T& value)
            : has_value_(true) {
            new (&storage_) T(value);
        }

        ~Node() {
            if (has_value_) {
                value().~T();
            }
        }

        [[nodiscard]] T& value() {
            return *std::launder(reinterpret_cast<T*>(&storage_));
        }

        std::atomic<Node*> next{nullptr};

      private:
        typename std::aligned_storage_t<sizeof(T), alignof(T)> storage_{};
        bool has_value_ = false;
    };

    Node* sentinel_ = nullptr;
    std::atomic<Node*> head_{nullptr};
    Node* tail_ = nullptr;
    std::atomic<std::size_t> size_{0};
};

} // namespace sandbox::voxel::concurrency

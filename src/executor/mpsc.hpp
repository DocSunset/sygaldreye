#pragma once
#include <atomic>
#include <utility>
#include <vector>

namespace syg::executor {

// The queue mapping's machinery (TCF-1 row: never drops, never duplicates,
// FIFO): producers push lock-free onto an atomic list; the consumer takes
// the whole list at the boundary and reverses it — FIFO in linearization
// order. Producers allocate; the consumer frees at control rate only.
template <class T>
class mpsc {
 public:
  ~mpsc() {
    for (auto* n = head_.load(); n;) {
      auto* next = n->next;
      delete n;
      n = next;
    }
  }

  void push(T v) {  // any thread, lock-free
    auto* n = new node{std::move(v), head_.load(std::memory_order_relaxed)};
    while (!head_.compare_exchange_weak(n->next, n, std::memory_order_release,
                                        std::memory_order_relaxed)) {
    }
  }

  // consumer thread only: everything pushed so far, oldest first
  void drain(std::vector<T>& out) {
    node* n = head_.exchange(nullptr, std::memory_order_acquire);
    std::size_t start = out.size();
    for (; n;) {
      out.push_back(std::move(n->value));
      auto* next = n->next;
      delete n;
      n = next;
    }
    for (std::size_t i = start, j = out.size(); i + 1 < j--; ++i)
      std::swap(out[i], out[j]);
  }

 private:
  struct node {
    T value;
    node* next;
  };
  std::atomic<node*> head_{nullptr};
};

}  // namespace syg::executor

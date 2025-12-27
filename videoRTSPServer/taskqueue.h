#pragma once
#include <cstddef>
#include <mutex>
#include <queue>

template <typename T>
class TaskQueue {
 public:
  TaskQueue() = default;
  ~TaskQueue() = default;
  bool empty() {
    std::lock_guard<std::mutex> lock(mtx_);
    return queue_.empty();
  }
  size_t size() {
    std::lock_guard<std::mutex> lock(mtx_);
    return queue_.size();
  }
  void enqueue(const T& item) {
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.emplace(item);
  }
  bool dequeue(T& item) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (queue_.empty()) {
      return false;
    }
    item = std::move(queue_.front());
    queue_.pop();
    return true;
  }

 private:
  std::queue<T> queue_;
  std::mutex mtx_;
};

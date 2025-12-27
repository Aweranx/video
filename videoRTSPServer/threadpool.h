#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "singleton.h"
#include "taskqueue.h"

class ThreadPool : public Singleton<ThreadPool> {
 public:
  friend class Singleton<ThreadPool>;
  // return future<type of f>, res.get() to get result
  template <typename F, typename... Args>
  auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    using func_type = decltype(f(args...))();
    // bind function and its args to get future result
    auto task_ptr = std::make_shared<std::packaged_task<func_type>>(
        [f, args...]() { return f(args...); });
    // wrap packaged task to be uniformed void() to fit in task queue
    std::function<void()> wrapper_func = [task_ptr]() { (*task_ptr)(); };
    tasks_.enqueue(wrapper_func);
    // notify one worker to process task
    condition_lock_.notify_one();
    return task_ptr->get_future();
  }
  void shutdown() {
    if (shutdown_) {
      return;
    }
    shutdown_ = true;
    // notify all worker to exit
    condition_lock_.notify_all();
    for (auto& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }
  ~ThreadPool() {
    if (!shutdown_) {
      shutdown();
    }
  }

 private:
  TaskQueue<std::function<void()>> tasks_;
  std::vector<std::thread> workers_;
  bool shutdown_;
  std::mutex mtx_;
  std::condition_variable condition_lock_;

  ThreadPool(size_t numThreads = 6) : shutdown_(false), workers_(numThreads) {
    init();
  }

  

  // 6 cores thread pool
  void init() {
    for (size_t i = 0; i < workers_.size(); ++i) {
      workers_[i] = std::thread([this, i]() {
        std::function<void()> func;
        bool dequeued = false;
        while (!shutdown_) {
          // create a new block to fetch task
          // to avoid holding lock while executing task
          {
            std::unique_lock<std::mutex> lcok(mtx_);
            if (tasks_.empty()) {
              // wait until there is a task or shutdown signal
              condition_lock_.wait(
                  lcok, [this]() { return shutdown_ || !tasks_.empty(); });
            }
            if (shutdown_ && tasks_.empty()) {
              return;
            }
            dequeued = tasks_.dequeue(func);
          }
          if (dequeued) {
            func();
          }
        }
      });
    }
  }
};

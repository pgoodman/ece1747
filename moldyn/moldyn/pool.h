/* Copyright 2014 Peter Goodman, all rights reserved. */


#ifndef MOLDYN_POOL_H_
#define MOLDYN_POOL_H_

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  explicit ThreadPool(size_t num_threads)
      : num_pending_tasks(0),
        done(false) {
    for (auto i = 0UL; i < num_threads; ++i) {
      auto worker = [this] (void) {
        Run();
      };
      threads.push_back(std::thread(worker));
    }
  }

  ~ThreadPool(void) {
    done = true;
    has_task.notify_all();
    for (auto &thread : threads) {
      thread.join();
    }
  }

  template <typename Func>
  inline void Enqueue(Func func) {
    task_queue_mutex.lock();
    auto signal = tasks.empty();
    ++num_pending_tasks;
    tasks.push_back(func);
    task_queue_mutex.unlock();
    has_task.notify_one();
  }

  void Barrier(void) {
    std::unique_lock<std::mutex> locker(pending_tasks_mutex);
    all_tasks_done.wait(locker);
  }

 private:
  ThreadPool(void) = delete;

  void Run(void) {
    for (;;) {
      std::function<void(void)> task;
      auto wake_up_barrier = false;
      {
        std::unique_lock<std::mutex> locker(task_queue_mutex);
        has_task.wait(locker, [this] (void) { return !tasks.empty(); });
        task = tasks[0];
        tasks.pop_front();

        wake_up_barrier = 0 == --num_pending_tasks;
      }
      task();

      if (wake_up_barrier) {
        all_tasks_done.notify_one();
      }
    }
  }

  std::vector<std::thread> threads;

  std::mutex task_queue_mutex;
  std::condition_variable has_task;

  int num_pending_tasks;
  std::mutex pending_tasks_mutex;
  std::condition_variable all_tasks_done;

  std::deque<std::function<void(void)>> tasks;

  bool done;
};

#endif  // MOLDYN_POOL_H_

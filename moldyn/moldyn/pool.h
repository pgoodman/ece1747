/* Copyright 2014 Peter Goodman, all rights reserved. */


#ifndef MOLDYN_POOL_H_
#define MOLDYN_POOL_H_

#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
class Semaphore{
  public:
  Semaphore(int initial_count):count(initial_count){}
  void Wait()
  {
    std::unique_lock<std::mutex> lk(lock);
    while(count < 1)
    {
      cv.wait(lk);
    }
    --count;
  }
  void Post()
  {
    std::unique_lock<std::mutex> lk(lock);
    ++count;
    cv.notify_one();
  }
  private:
    int count;
    std::mutex lock;
    std::condition_variable cv;
};

class ThreadPool {
 public:
  explicit ThreadPool(size_t num_threads):goSem(0),doneSem(0)
  {
    for (auto i = 0UL; i < num_threads; ++i) {
      auto worker = [this] (void) {
        Run();
      };
      threads.push_back(std::thread(worker));
    }
  }

  ~ThreadPool(void) {
    done = true;
    for (auto i = 0UL; i < threads.size(); ++i) {
      goSem.Post();
    }
    for (auto &thread : threads) {
      thread.join();
    }
  }

  template <typename Func>
  inline void Enqueue(Func func) {
    taskQueueMutex.lock();
    tasks.push_back(func);
    num_tasks_ready++;
    goSem.Post();
    //std::cout<<"goSem:post"<<std::endl;
    taskQueueMutex.unlock();
  }

  inline void AllThreadsDone()
  {
    // wait on doneCV num_threads number of times
    for(int i = 0; i < threads.size(); ++i)
    {
      doneSem.Wait();
      //std::cout<<"doneSem:wait"<<std::endl;
    }
  }
 
 private:
  ThreadPool(void) = delete;

  void Run(void) {
    for (;;) {
      //Wait on goCV
      goSem.Wait();
      if(done){
        return;
      }
//      std::cout<<"goSem:wait"<<std::endl;
      taskQueueMutex.lock();
      num_tasks_ready--;
      std::function<void(void)> task= tasks[0];
      tasks.pop_front();
      taskQueueMutex.unlock();
      task();

      //notify on doneCV.
      {
        doneSem.Post();
        //std::cout<<"domeSem:post"<<std::endl;
      }
    }
  }

  std::vector<std::thread> threads;
  Semaphore goSem;
  Semaphore doneSem;
  std::mutex goMutex;
  int num_tasks_ready;
  std::mutex doneMutex;
  int num_tasks_done;
  std::mutex taskQueueMutex;
  std::condition_variable goCv;
  std::condition_variable doneCv;
  std::deque<std::function<void(void)>> tasks;

  bool done;
};

#endif  // MOLDYN_POOL_H_

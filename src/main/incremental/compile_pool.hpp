#pragma once
#include <chrono>
#include <queue>
#include <thread>
#include <vector>
#include <tuple>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "config_parser.hpp"
#include "platform/taskset.hpp"

namespace rsl::testing::_impl_main {

class CompilePool {
  using output_type = std::pair<TestTU, ProcessResult>;

  std::vector<std::thread> workers;
  std::queue<TestTU> tasks;
  std::vector<output_type> results;

  std::mutex queueMutex;
  std::mutex resultMutex;
  std::condition_variable cv;
  std::atomic<bool> stop;

  std::atomic<size_t> submittedCount;
  std::atomic<size_t> completedCount;

public:
  explicit CompilePool(size_t numThreads = std::thread::hardware_concurrency()) : stop(false), submittedCount(0), completedCount(0) {
    auto num_cpu = std::thread::hardware_concurrency();
    // TODO hardware_concurrency might be 0
    for (size_t i = 0; i < numThreads; ++i) {
      workers.emplace_back([this, cpu=i%num_cpu] { 
        workerLoop(static_cast<int>(cpu));
      });
    }
  }

  ~CompilePool() {
    stop.store(true, std::memory_order_relaxed);
    cv.notify_all();
    for (auto& t : workers) {
      t.join();
    }
  }

  void submit(TestTU const& task) {
    {
      std::scoped_lock lock(queueMutex);
      tasks.push(task);
      submittedCount.fetch_add(1, std::memory_order_relaxed);
    }
    cv.notify_one();
  }

  void wait() {
    while (completedCount.load(std::memory_order_acquire) <
           submittedCount.load(std::memory_order_acquire)) {
      std::this_thread::yield();  // spin-wait
    }
  }

  void wait(auto&& on_update) {
    size_t lastCompleted = completedCount.load(std::memory_order_acquire);

    //! assumes we don't call submit from other threads while waiting
    size_t total = submittedCount.load(std::memory_order_acquire);

    while (lastCompleted < total) {
        size_t currentCompleted = completedCount.load(std::memory_order_acquire);

        if (currentCompleted != lastCompleted) {
            on_update(currentCompleted, total);
            lastCompleted = currentCompleted;
        }

        std::this_thread::yield();
    }
  }

  std::vector<output_type> collect() {
    std::vector<output_type> output;
    {
      std::scoped_lock lock(resultMutex);
      output.swap(results);
    }
    submittedCount.store(0, std::memory_order_relaxed);
    completedCount.store(0, std::memory_order_relaxed);
    return output;
  }

private:
  void workerLoop(int cpu) {
    while (!stop.load(std::memory_order_relaxed)) {
      TestTU task;
      {
        std::unique_lock lock(queueMutex);
        if (tasks.empty()) {
          cv.wait(lock, [&] { return stop.load() || !tasks.empty(); });
          if (stop.load() && tasks.empty()) {
            return;
          }
        }

        task = std::move(tasks.front());
        tasks.pop();
      }
      // std::println("building {}", task.out_path.string());
      // auto start_time = std::chrono::steady_clock::now();
      output_type result = {task, run_on_cpu(cpu, task.invocation)};
      // auto end_time = std::chrono::steady_clock::now();
      // std::println("{} - {}", task.out_path.string(), std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time));
      {
        std::scoped_lock lock(resultMutex);
        results.push_back(std::move(result));
      }

      completedCount.fetch_add(1, std::memory_order_release);
    }
  }
};
}  // namespace rsl::testing::_impl_main
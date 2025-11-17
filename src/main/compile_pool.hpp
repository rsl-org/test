#pragma once
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

  std::vector<output_type> collect() {
    wait();
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

      output_type result = {task, run_on_cpu(cpu, task.invocation)};
      {
        std::scoped_lock lock(resultMutex);
        results.push_back(std::move(result));
      }

      completedCount.fetch_add(1, std::memory_order_release);
    }
  }
};
}  // namespace rsl::testing::_impl_main
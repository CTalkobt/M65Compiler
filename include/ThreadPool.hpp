#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <atomic>
#include <future>

// Phase 4b: Thread pool for parallel batch conversion
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};
    std::atomic<int> activeThreads{0};

    void workerThread() {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this] { return !tasks.empty() || stop; });

                if (stop && tasks.empty()) {
                    break;
                }

                if (tasks.empty()) {
                    continue;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }

            activeThreads++;
            try {
                task();
            } catch (...) {
                // Catch and suppress exceptions in worker threads
                // They should be handled by the task itself
            }
            activeThreads--;
        }
    }

public:
    // Initialize thread pool with specified number of workers
    ThreadPool(size_t numThreads = 0) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4;  // Default to 4 if detection fails
        }

        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back(&ThreadPool::workerThread, this);
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Enqueue a task for execution
    template<typename F>
    void enqueue(F&& func) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) {
                throw std::runtime_error("Cannot enqueue task on stopped thread pool");
            }
            tasks.emplace(std::forward<F>(func));
        }
        condition.notify_one();
    }

    // Wait for all queued tasks to complete
    void waitAll() {
        std::unique_lock<std::mutex> lock(queueMutex);
        condition.wait(lock, [this] { return tasks.empty() && activeThreads == 0; });
    }

    // Get number of worker threads
    size_t getWorkerCount() const {
        return workers.size();
    }

    // Get number of pending tasks
    size_t getPendingTasks() {
        std::unique_lock<std::mutex> lock(queueMutex);
        return tasks.size();
    }

    // Get number of active threads currently processing
    int getActiveThreads() const {
        return activeThreads.load();
    }
};

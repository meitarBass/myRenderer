#include "../Utils/ThreadPool.h"

ThreadPool::ThreadPool(size_t threads) {
    for (size_t i = 0; i < threads; i++) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });

                    if (this->stop && this->tasks.empty()) {
                        return;
                    }

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();
                activeTasks.fetch_sub(1, std::memory_order_relaxed);

                if (activeTasks == 0 && tasks.empty()) {
                    finishedCondition.notify_one();
                }
            }
        });
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.push(std::move(task));
        activeTasks.fetch_add(1, std::memory_order_relaxed);
    }

    condition.notify_one();
}

void ThreadPool::waitFinished() {
    std::unique_lock<std::mutex> lock(queueMutex);

    finishedCondition.wait(lock, [this]() {
        return tasks.empty() && (activeTasks == 0);
    });
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();

    for(std::thread &worker: workers) {
        worker.join();
    }
}
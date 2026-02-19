#ifndef RENDERER_THREADPOOL_H
#define RENDERER_THREADPOOL_H

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>


class ThreadPool {
public:
    static ThreadPool& instance() {
        static ThreadPool pool(std::thread::hardware_concurrency());
        return pool;
    }

    void enqueue(std::function<void()> task);
    void waitFinished();

private:
    explicit ThreadPool(size_t threads);
    ~ThreadPool();

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable finishedCondition;

    bool stop = false;
    std::atomic<int> activeTasks = 0;
};


#endif //RENDERER_THREADPOOL_H
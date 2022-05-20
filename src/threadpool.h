#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <vector>
#include <queue>
#include <QDebug>

class ThreadPool
{
public:
    using Task = std::function<void()>;

    explicit ThreadPool(int num);

    ~ThreadPool();

    void start();

    void stop();

    bool append_task(const Task& task);

private:
    void work();

public:
    // disable copy and assign construct
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;

private:
    std::atomic_bool _is_running; // thread pool manager status
    std::mutex _mtx;
    std::condition_variable _cond;
    int _thread_num;
    std::vector<std::thread> _threads;
    std::queue<Task> _tasks;
};

#endif // THREADPOOL_H

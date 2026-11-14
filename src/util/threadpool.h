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

    bool append_task(const Task &task);

private:
    void work();

public:
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool &operator=(const ThreadPool &other) = delete;

private:
    std::atomic_bool m_is_running; // thread pool manager status
    std::mutex m_mtx;
    std::condition_variable m_cond;
    int m_thread_num;
    std::vector<std::thread> m_threads;
    std::queue<Task> m_tasks;
};

#endif // THREADPOOL_H

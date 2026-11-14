#include "threadpool.h"

ThreadPool::ThreadPool(int num): m_is_running(false), m_thread_num(num) {}

ThreadPool::~ThreadPool()
{
    if (m_is_running) stop();
}

void ThreadPool::start()
{
    m_is_running = true;

    for (int i = 0; i < m_thread_num; i++)
        m_threads.emplace_back(std::thread(&ThreadPool::work, this));
}

void ThreadPool::stop()
{
    {
        // stop thread pool, should notify all threads to wake
        std::unique_lock<std::mutex> lk(m_mtx);
        m_is_running = false;
        m_cond.notify_all(); // must do this to avoid thread block
    }

    for (std::thread &t : m_threads) {
        if (t.joinable())
            t.join();
    }
}

bool ThreadPool::append_task(const ThreadPool::Task &task)
{
    if (!m_is_running || m_tasks.size() > 80) return false;
    std::unique_lock<std::mutex> lk(m_mtx);
    m_tasks.push(task);
    m_cond.notify_one();
    return true;
}

void ThreadPool::work()
{
//    printf("begin work thread: %d\n", std::this_thread::get_id());

    while (m_is_running) {
        Task task;
        {
            std::unique_lock<std::mutex> lk(m_mtx);
            if (!m_tasks.empty()) {
                // if tasks not empty,
                // must finish the task whether thread pool is running or not
                task = m_tasks.front();
                m_tasks.pop();
            }
            else if (m_is_running && m_tasks.empty())
                m_cond.wait(lk);
        }

        if (task) task();
    }

//    printf("end work thread: %d\n", std::this_thread::get_id());
}

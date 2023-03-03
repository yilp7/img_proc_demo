#include "threadpool.h"

ThreadPool::ThreadPool(int num): _is_running(false), _thread_num(num) {}

ThreadPool::~ThreadPool()
{
    if (_is_running) stop();
}

void ThreadPool::start()
{
    _is_running = true;

    // start threads
    for (int i = 0; i < _thread_num; i++)
        _threads.emplace_back(std::thread(&ThreadPool::work, this));
}

void ThreadPool::stop()
{
    {
        // stop thread pool, should notify all threads to wake
        std::unique_lock<std::mutex> lk(_mtx);
        _is_running = false;
        _cond.notify_all(); // must do this to avoid thread block
    }

    // terminate every thread job
    for (std::thread& t : _threads)
    {
        if (t.joinable())
            t.join();
    }
}

bool ThreadPool::append_task(const ThreadPool::Task &task)
{
    if (!_is_running || _tasks.size() > 80) return false;
    std::unique_lock<std::mutex> lk(_mtx);
    _tasks.push(task);
    _cond.notify_one(); // wake a thread to to the task
    return true;
}

void ThreadPool::work()
{
//    printf("begin work thread: %d\n", std::this_thread::get_id());

    // every thread will compete to pick up task from the queue to do the task
    while (_is_running)
    {
        Task task;
        {
            std::unique_lock<std::mutex> lk(_mtx);
            if (!_tasks.empty())
            {
                // if tasks not empty,
                // must finish the task whether thread pool is running or not
                task = _tasks.front();
                _tasks.pop(); // remove the task
            }
            else if (_is_running && _tasks.empty())
                _cond.wait(lk);
        }

        if (task) task(); // do the task
    }

//    printf("end work thread: %d\n", std::this_thread::get_id());
}

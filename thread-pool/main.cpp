#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

using Task = std::function<void()>;

class ThreadPool
{
public:
    ThreadPool(const unsigned int size = std::thread::hardware_concurrency())
        : m_size{size}
    {
        for (int i = 0; i < size; i++)
        {
            m_pool.push_back(std::thread([this] { Run_(); }));
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool()
    {
        for(unsigned int i = 0; i < m_size; ++i)
        {
            submit([&] { m_is_done_ = true; });
        }

        for (auto& thd : m_pool)
        {
            if (thd.joinable())
                thd.join();
        }
    }

    void submit(Task task)
    {
        m_tasks.push(task);
    }

private:
    void Run_()
    {
        Task task;
        while (!m_is_done_)
        {
            m_tasks.pop(task);            
            task();
        }
    }

    const unsigned int m_size;
    std::vector<std::thread> m_pool;
    ThreadSafeQueue<Task> m_tasks;
    std::atomic<bool> m_is_done_{false};
};

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    {
        ThreadPool thd_pool{6};

        thd_pool.submit([] { background_work(1, "Text#1", 100ms); });
        thd_pool.submit([] { background_work(1, "Text#2", 150ms); });

        for (int i = 3; i < 20; ++i)
            thd_pool.submit([i]() { background_work(i, "Text#" + std::to_string(i), 150ms); });
    }

    std::cout << "Main thread ends..." << std::endl;
}

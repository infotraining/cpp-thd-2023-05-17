#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

using Task = std::function<void()>;
//using Task = folly::Function<void()>;

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

    template <typename Callable>
    auto submit(Callable&& callable)
    {
        using ResultT = decltype(callable());
        //std::packaged_task<ResultT()> pt{std::forward<Callable>(callable)}; // This doesn't work - pt in noncopyable
        auto pt = std::make_shared<std::packaged_task<ResultT()>>(std::forward<Callable>(callable));
        std::future<ResultT> f = pt->get_future();

        m_tasks.push([pt] () mutable { (*pt)(); });

        return f;
    }

    ~ThreadPool()
    {
        for (unsigned int i = 0; i < m_size; ++i)
        {
            submit([this] { m_is_done_ = true; });
        }

        for (auto& thd : m_pool)
        {
            if (thd.joinable())
                thd.join();
        }
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

int calculate_square(int x)
{
    std::cout << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 3 == 0)
        throw std::runtime_error("Error#3");

    return x * x;
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

        auto f_result_41 = thd_pool.submit([] { return calculate_square(41); });

        std::vector<std::tuple<int, std::future<int>>> fsquares;
        for (int i = 42; i < 55; ++i)
        {
            fsquares.push_back(std::tuple{i, thd_pool.submit([i] { return calculate_square(i); })});
        }

        for (auto& fs : fsquares)
        {
            auto& [n, square_n] = fs;
            try
            {
                auto result = square_n.get();
                std::cout << n << " * " << n << " = " << result << "\n";
            }
            catch (const std::runtime_error& e)
            {
                std::cout << n << " : " << e.what() << "\n";
            }
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}

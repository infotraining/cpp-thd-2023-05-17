#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

namespace Atomics
{

    class Data
    {
        std::vector<int> data_;
        std::atomic<bool> is_data_ready_{false};

    public:
        void read()
        {
            std::cout << "Start reading..." << std::endl;
            data_.resize(100);

            std::random_device rnd;
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            std::this_thread::sleep_for(2s);
            std::cout << "End reading..." << std::endl;
            // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            // is_data_ready_ = true;
            is_data_ready_.store(true, std::memory_order_release); //------------------------------
        }

        void process(int id)
        {
            // while (!is_data_ready_) {} // busy wait // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

            while (!is_data_ready_.load(std::memory_order_acquire)) { } // busy wait //------------------------------
            long sum = std::accumulate(begin(data_), end(data_), 0L);

            std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };

} // namespace Atomics

namespace CVs
{
    class Data
    {
        std::vector<int> data_;
        bool is_data_ready_{false};
        std::condition_variable cv_data_ready_;
        std::mutex mtx_data_ready_;

    public:
        void read()
        {
            std::cout << "Start reading..." << std::endl;
            data_.resize(100);

            std::random_device rnd;
            std::generate(begin(data_), end(data_), [&rnd] { return rnd() % 1000; });
            std::this_thread::sleep_for(2s);
            std::cout << "End reading..." << std::endl;
            // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            {
                std::lock_guard lk{mtx_data_ready_};
                is_data_ready_ = true;
            }
            cv_data_ready_.notify_all();
        }

        void process(int id)
        {
            {
                std::unique_lock lk{mtx_data_ready_};
                // while (!is_data_ready_)
                // {
                //     cv_data_ready_.wait(lk);
                // }
                cv_data_ready_.wait(lk, [this] { return is_data_ready_; });
            }

            long sum = std::accumulate(begin(data_), end(data_), 0L);

            std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
        }
    };

} // namespace CVs

int main()
{
    using namespace CVs;

    {
        Data data;
        std::jthread thd_producer{[&data] {
            data.read();
        }};
        std::jthread thd_consumer_1{[&data] {
            data.process(1);
        }};
        std::jthread thd_consumer_2{[&data] {
            data.process(2);
        }};
    }

    std::cout << "END of main..." << std::endl;
}

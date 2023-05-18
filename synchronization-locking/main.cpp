#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

using namespace std::literals;

void run(int& value, std::mutex& mtx_value)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        // mtx_value.lock(); // begin of CS
        // ++value;
        // mtx_value.unlock(); // end of CS

        {            
            std::lock_guard lk{mtx_value}; // begin of CS - mtx_value.lock()            
            ++value;
        } // end of CS - mtx_value.unlock()
    }
}

template <typename T, typename TMutex = std::mutex>
struct SynchronizedValue
{
    T value;
    TMutex mtx_value;

    void lock()
    {
        mtx_value.lock();
    }

    void unlock()
    {
        mtx_value.unlock();
    }
};

void run(SynchronizedValue<int>& counter)
{
    for(int i = 0; i < 10'000'000; ++i)
    {
        // mtx_value.lock(); // begin of CS
        // ++value;
        // mtx_value.unlock(); // end of CS

        {            
            std::lock_guard lk{counter}; // begin of CS - mtx_value.lock()            
            ++counter.value;
        } // end of CS - mtx_value.unlock()
    }
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;

    // int counter = 0;
    // std::mutex mtx_counter;
    SynchronizedValue<int> counter;

    {
        std::jthread thd_1{[&counter] { run(counter); }};
        std::jthread thd_2{[&counter] { run(counter); }};
    }

    std::cout << "counter: " << counter.value << "\n";

    std::cout << "Main thread ends..." << std::endl;
}

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <random>
#include <numeric>

using namespace std::literals;

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
        //XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        //is_data_ready_ = true;
        is_data_ready_.store(true, std::memory_order_release); //------------------------------        
    }

    void process(int id)
    {   
        // while (!is_data_ready_) {} // busy wait // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
         
        while (!is_data_ready_.load(std::memory_order_acquire)) {} // busy wait //------------------------------   
        long sum = std::accumulate(begin(data_), end(data_), 0L);

        std::cout << "Id: " << id << "; Sum: " << sum << std::endl;
    }
};

int main()
{
    {
        Data data;
        std::jthread thd_producer{ [&data] { data.read(); }};
        std::jthread thd_consumer_1{[&data] { data.process(1); }};
        std::jthread thd_consumer_2{[&data] { data.process(2); }};
    }

    std::cout << "END of main..." << std::endl;
}

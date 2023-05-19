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

void save_to_file(const std::string& filename)
{
    std::cout << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    std::cout << "File saved: " << filename << std::endl;
}

int main()
{
    std::future<int> f1 = std::async(std::launch::async, &calculate_square, 13);
    std::future<int> f2 = std::async(std::launch::deferred, &calculate_square, 5);

    std::vector<std::future<int>> future_squares;
    future_squares.push_back(std::move(f1));
    future_squares.push_back(std::move(f2));
    future_squares.push_back(std::async(std::launch::async, &calculate_square, 3));

    std::future<void> f4 = std::async(std::launch::async, &save_to_file, "data.txt");

    while (future_squares[0].wait_for(100ms) != std::future_status::ready)
    {
        std::cout << "I am waiting..." << std::endl;
    }

    for (auto& fs : future_squares)
    {
        try
        {
            auto result = fs.get();
            std::cout << "f3: " << result << std::endl;
        }
        catch (const std::runtime_error& excpt)
        {
            std::cout << "Caught: " << excpt.what() << std::endl;
        }
    }

    f4.wait();

    std::cout << "////////////////////////////////////////////////////////" << std::endl;
    auto a1 = std::async(std::launch::async, &save_to_file, "data1.txt");
    auto a2 = std::async(std::launch::async, &save_to_file, "data2.txt");
    auto a3 = std::async(std::launch::async, &save_to_file, "data3.txt");
    auto a4 = std::async(std::launch::async, &save_to_file, "data4.txt");
}

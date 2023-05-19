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

class SquareCalculator
{
    std::promise<int> promise_;
public:
    void calculate(int x)
    {
        try
        {
            int result = calculate_square(x);
            promise_.set_value(result);
        }
        catch(...)
        {
            promise_.set_exception(std::current_exception());
        }        
    }

    std::future<int> get_future()
    {
        return promise_.get_future();
    }
};

template <typename Callable>
auto spawn_task(Callable&& callable)
{
    using ResultT = decltype(callable());
    std::packaged_task<ResultT()> pt{std::forward<Callable>(callable)};
    std::future<ResultT> f = pt.get_future();

    std::thread thd{std::move(pt)}; // async call of function
    thd.detach();

    return f;
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

    std::cout << "////////////////////////////////////////////////////////" << std::endl;

    SquareCalculator calc;

    auto f_result = calc.get_future();

    {
        std::jthread thd{ [&calc] { calc.calculate(13); }};

        std::cout << "f_result: " << f_result.get() << "\n";
    }

    std::cout << "////////////////////////////////////////////////////////" << std::endl;
    spawn_task([] { save_to_file("data1.txt");} );
    spawn_task([] { save_to_file("data2.txt");} );
    spawn_task([] { save_to_file("data3.txt");} );
    auto f_task = spawn_task([] { return calculate_square(41); });
    std::cout << "f_task:" << f_task.get() << "\n";
}

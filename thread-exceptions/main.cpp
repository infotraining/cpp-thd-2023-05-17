#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

template <typename T>
struct ThreadResult
{
    T value;
    std::exception_ptr eptr;    

    T get()
    {
        if (eptr)
        {
            std::rethrow_exception(eptr);
        }

        return std::move(value);
    }
};

void background_work(size_t id, const std::string& text, ThreadResult<char>& result)
{
    try
    {
        std::cout << "bw#" << id << " has started..." << std::endl;

        for (const auto& c : text)
        {
            std::cout << "bw#" << id << ": " << c << std::endl;

            std::this_thread::sleep_for(100ms);
        }

        result.value = text.at(5); // potential exception
    }
    catch (...)
    {
        result.eptr = std::current_exception();
        std::cout << "Caught an exception!" << std::endl;
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
try
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    
    std::vector<ThreadResult<char>> results(3);

    {
        std::vector<std::jthread> threads;
        threads.push_back(std::jthread{&background_work, 1, "HelloWorld!", std::ref(results[0])});
        threads.push_back(std::jthread{&background_work, 2, "Hell", std::ref(results[1])});
        threads.push_back(std::jthread{&background_work, 3, "Hellish", std::ref(results[2])});
    } // join


    for(auto& result : results)
    {
        try
        {
            char c = result.get();
            std::cout << "Result: " << c << std::endl;
        }
        catch(const std::out_of_range& excpt)
        {
            std::cout << "Caught: " << excpt.what() << std::endl;
        }
        catch(const std::runtime_error& excpt)
        {
            std::cout << "Caught runtime error: " << excpt.what() << std::endl;
        }
        catch(...)
        {
            std::cout << "Caught an exception!" << std::endl;
        }
    }

    std::cout << "Main thread ends..." << std::endl;
}
catch (...)
{
    std::cout << "Caught an exceptions..." << std::endl;
}

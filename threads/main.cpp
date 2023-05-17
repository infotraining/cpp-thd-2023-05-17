#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

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

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_{id}
        , text_{std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay) const
    {
        std::cout << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            std::cout << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id_ << " is finished..." << std::endl;
    }
};

void may_throw()
{
    // throw std::runtime_error("Error#13");
}

std::thread create_thread(const std::string& text)
{
    static int id_gen = 665;
    return std::thread{&background_work, ++id_gen, text, 100ms};
}

void move_semantics_for_threads()
{
    std::thread thd = create_thread("Move Semantics");

    std::vector<std::thread> working_threads;

    working_threads.push_back(std::move(thd));
    working_threads.push_back(create_thread("Second"));
    working_threads.push_back(create_thread("Third"));
    working_threads.push_back(create_thread("Fourth"));

    working_threads[0].detach();

    for(auto& wt : working_threads)    
        if (wt.joinable())
            wt.join();
}

void background_task(std::stop_token cancel_token, size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        if (cancel_token.stop_requested())
        {
            std::cout << "Stop requested..." << std::endl;
            return;
        }

        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

void cooperative_cancelling_threads()
{
    std::stop_source token_source;
    std::stop_token tkn_1 = token_source.get_token();

    std::jthread thd_1{[&token_source] { background_task(token_source.get_token(), 1, "THD1", 200ms); } };
    std::jthread thd_2{&background_task, tkn_1, 2, "THD2", 250ms};

    std::jthread stopper{[&token_source] { 
        std::this_thread::sleep_for(1s);
        token_source.request_stop();
    }};

    background_task(tkn_1, 42, "MAINTHD", 300ms);

    std::cout << "End of scope..." << std::endl;
}

int main()
{
    std::cout << "No of cores: " << std::thread::hardware_concurrency() << std::endl;

    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    std::thread thd_empty;
    std::cout << "thd_empty.id: " << thd_empty.get_id() << std::endl;

    {
        std::thread thd_1{&background_work, 1, std::cref(text), 1500ms};
        std::jthread thd_2{BackgroundWork{2, "Function object"}, 200ms};
        std::jthread thd_3{[&text] {
            background_work(3, text, 100ms);
        }};

        thd_empty = std::thread{&background_work, 4, "Not empty", 100ms};

        may_throw();

        thd_1.detach(); // thd_1 is deamon
        thd_empty.join();

        assert(thd_1.joinable() == false);

        if (thd_1.joinable())
            thd_1.join();
    } // implicit join on jthread instances

    ///////////////////////////////////////////////

    const std::vector source = {1, 2, 3, 4, 5, 6};
    std::vector<int> target_1(source.size());
    std::vector<int> target_2(source.size());

    {
        std::thread thd_copy_1{[&source, &target_1]() {
            std::copy(source.begin(), source.end(), target_1.begin());
        }};

        std::jthread thd_copy_2{[&source, &target_2]() {
            std::copy(source.begin(), source.end(), target_2.begin());
        }};

        thd_copy_1.join();

        for(const auto& item : target_1)
            std::cout << item << " ";
        std::cout << "\n";
    } // implicit join for thd_copy_2

    for(const auto& item : target_2)
        std::cout << item << " ";
    std::cout << "\n";

    move_semantics_for_threads();   

    std::cout << "--------------------------------------" << std::endl;

    cooperative_cancelling_threads();

    std::cout << "--------------------------------------" << std::endl;
    
    std::jthread thd_printer{&background_task, 888, "PRINT", 200ms};

    std::this_thread::sleep_for(500ms);

    thd_printer.request_stop();
    
    std::cout << "Main thread ends..." << std::endl;    
}

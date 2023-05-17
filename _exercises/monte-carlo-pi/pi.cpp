#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

using namespace std;

const long N = 100'000'000;

void calc_hits(const long N, long& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    for (long n = 0; n < N; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            hits++;
    }
}

void single_thread_pi()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    //////////////////////////////////////////////////////////////////////////////
    // single thread

    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    long hits = 0;

    calc_hits(N, hits);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

void ver_1()
{
    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    long hits = 0;
    const unsigned int num_of_cores = std::thread::hardware_concurrency();

    std::vector<long> hits_vec(num_of_cores, 0);
    std::vector<std::thread> threads;

    for (int i = 0; i < num_of_cores; ++i)
    {
        threads.push_back(std::thread{&calc_hits, N / num_of_cores, std::ref(hits_vec[i])});
    }

    for (auto& thread : threads)
        if (thread.joinable())
            thread.join();

    hits = std::accumulate(hits_vec.begin(), hits_vec.end(), 0L);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

void calc_hits(long N, std::atomic<long>& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1);

    for (long n = 0; n < N; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
        {
            hits++;
        }
    }
}

void ver_2()
{
    long no_of_cores = std::thread::hardware_concurrency();
    long chunk_size = N / no_of_cores;
    cout << "no_of_cores: " << no_of_cores << endl;

    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();
        
    atomic<long> hits = 0;

    vector<thread> v(no_of_cores);

    for (int i = 0; i < no_of_cores; ++i)
    {
        // v[i] = std::thread(static_cast<void(*)(long, std::atomic<long>&)>(calc_hits), chunk_size, ref(hits));
        v[i] = std::thread{[chunk_size, &hits] { calc_hits(chunk_size, hits); }};
    }

    for (auto& thd : v)
    {
        thd.join();
    }

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

int main()
{
    single_thread_pi();

    std::cout << "///////////////////////////////////////////////////" << std::endl;

    ver_1();

    std::cout << "///////////////////////////////////////////////////" << std::endl;

    ver_2();
}

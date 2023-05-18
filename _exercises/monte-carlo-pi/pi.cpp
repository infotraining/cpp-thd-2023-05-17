#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>
#include <latch>

/*******************************************************
 * https://academo.org/demos/estimating-pi-monte-carlo
 * *****************************************************/

using namespace std;

const uintmax_t N = 100'000'000;

void calc_hits(const uintmax_t count, uintmax_t& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    for (uintmax_t n = 0; n < count; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            hits++;
    }
}

void calc_hits_with_latch(const uintmax_t count, uintmax_t& hits, std::latch& starter)
{
    starter.arrive_and_wait();

    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    for (uintmax_t n = 0; n < count; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            hits++;
    }
}

void calc_hits_with_local(const uintmax_t count, uintmax_t& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    uintmax_t local_hits = 0;
    for (long n = 0; n < count; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            local_hits++;
    }
    hits += local_hits;
}

struct Hits
{
    alignas(std::hardware_destructive_interference_size) uintmax_t value;
};

void calc_hits_with_padding(const uintmax_t count, Hits& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1.0);

    for (uintmax_t n = 0; n < count; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
            hits.value++;
    }
}

void single_thread_pi()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    //////////////////////////////////////////////////////////////////////////////
    // single thread

    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;

    calc_hits(N, hits);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

void pi_many_threads_hot_loop()
{
    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;
    const unsigned int number_of_cores = std::thread::hardware_concurrency();
    const uintmax_t chunk_size = N / number_of_cores;

    std::vector<uintmax_t> hits_vec(number_of_cores, 0);
    std::vector<std::thread> threads;

    for (int i = 0; i < number_of_cores; ++i)
    {
        threads.push_back(std::thread{&calc_hits, chunk_size, std::ref(hits_vec[i])});
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

void calc_hits(uintmax_t N, std::atomic<uintmax_t>& hits)
{
    std::mt19937_64 rnd_gen(std::hash<std::thread::id>{}(this_thread::get_id()));
    std::uniform_real_distribution<double> rnd(0, 1);

    uintmax_t local_hits{};
    for (uintmax_t n = 0; n < N; ++n)
    {
        double x = rnd(rnd_gen);
        double y = rnd(rnd_gen);
        if (x * x + y * y < 1)
        {
            local_hits++;
        }
    }
    
    //hits += local_hits;
    hits.fetch_add(1, std::memory_order_relaxed);
}

void pi_with_atomic()
{
    const uintmax_t number_of_cores = std::thread::hardware_concurrency();
    const uintmax_t chunk_size = N / number_of_cores;
    cout << "number_of_cores: " << number_of_cores << endl;

    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    std::atomic<uintmax_t> hits = 0;

    vector<thread> v(number_of_cores);

    for (int i = 0; i < number_of_cores; ++i)
    {
        // v[i] = std::thread(static_cast<void(*)(uintmax_t, std::atomic<uintmax_t>&)>(calc_hits), chunk_size, ref(hits));
        v[i] = std::thread{[chunk_size, &hits] {
            calc_hits(chunk_size, hits);
        }};
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

void pi_local_counter()
{
    const uintmax_t number_of_cores = std::thread::hardware_concurrency();
    auto chunk_size = N / number_of_cores;
    uintmax_t hits = 0;

    cout << "number_of_cores: " << number_of_cores << endl;

    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    std::vector<uintmax_t> hits_vec(number_of_cores);
    std::vector<std::thread> threads(number_of_cores);
    for (auto idx = 0; idx < number_of_cores; ++idx)
    {
        threads[idx] = std::thread{
            [&hits_vec, idx, chunk_size] {
                calc_hits_with_local(chunk_size, hits_vec[idx]);
            }};
    }
    for (auto idx = 0; idx < number_of_cores; ++idx)
    {
        threads[idx].join();
    }

    hits = std::accumulate(hits_vec.begin(), hits_vec.end(), 0ULL);

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;    
}

void pi_with_padding()
{
    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;
    const unsigned int number_of_cores = std::thread::hardware_concurrency();
    const uintmax_t chunk_size = N / number_of_cores;

    std::vector<Hits> hits_vec(number_of_cores);
    std::vector<std::thread> threads;

    for (int i = 0; i < number_of_cores; ++i)
    {
        threads.push_back(std::thread{&calc_hits_with_padding, chunk_size, std::ref(hits_vec[i])});
    }

    for (auto& thread : threads)
        if (thread.joinable())
            thread.join();

    hits = std::accumulate(hits_vec.begin(), hits_vec.end(), 0L, [](uintmax_t total, Hits hits) { return total + hits.value; });

    const double pi = static_cast<double>(hits) / N * 4;

    const auto end = chrono::high_resolution_clock::now();
    const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Pi = " << pi << endl;
    cout << "Elapsed = " << elapsed_time << "ms" << endl;
}

void pi_many_threads_latch()
{
    cout << "Pi calculation started!" << endl;
    const auto start = chrono::high_resolution_clock::now();

    uintmax_t hits = 0;
    const unsigned int number_of_cores = 512;
    std::latch starter{number_of_cores};
    const uintmax_t chunk_size = N / number_of_cores;

    std::vector<uintmax_t> hits_vec(number_of_cores, 0);
    std::vector<std::thread> threads;

    for (int i = 0; i < number_of_cores; ++i)
    {
        threads.push_back(std::thread( [i, chunk_size, &hits_vec, &starter](){ calc_hits_with_latch(chunk_size, hits_vec[i], starter); }));
    }

    std::cout << "No of threads:" << threads.size() << "\n";

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

int main()
{
    single_thread_pi();

    std::cout << "///////////////////////////////////////////////////" << std::endl;

    pi_many_threads_hot_loop();

    std::cout << "///////////////////////////////////////////////////" << std::endl;

    pi_with_atomic();

    std::cout << "///////////////////////////////////////////////////" << std::endl;

    pi_local_counter();

    std::cout << "///////////////////////////////////////////////////" << std::endl;

    pi_with_padding();

    std::cout << "///////////////////////////////////////////////////" << std::endl;

    pi_many_threads_latch();
}
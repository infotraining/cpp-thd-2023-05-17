#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

class SpinLockMutex
{
    std::atomic_flag flag_;

public:
    SpinLockMutex()
        : flag_{ATOMIC_FLAG_INIT}
    {
    }

    void lock()
    {
        while (flag_.test_and_set(std::memory_order_acquire))
            continue;
    }
    
    void unlock()
    {
        flag_.clear(std::memory_order_release);
    }
};

using MutexType = SpinLockMutex;

MutexType mtx;
long counter = 0;

void increase()
{
    for (int i = 0; i < 100000; ++i)
    {
        lock_guard<MutexType> l(mtx);
        ++counter;
        if (counter == 1000)
            return;
    }
}

int main()
{
    cout << "Race Condition" << endl;
    //cout << "Is this lock-free? " << counter.is_lock_free() << endl;

    vector<thread> thds;

    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < 2; ++i)
        thds.emplace_back(&increase);

    for (auto& th : thds)
        th.join();

    auto end = chrono::high_resolution_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(end - start).count() << " us" << endl;

    cout << "Counter = " << counter << endl;
}

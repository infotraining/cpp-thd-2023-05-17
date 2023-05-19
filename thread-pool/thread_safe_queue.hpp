#ifndef THREAD_SAFE_QUEUE_HPP
#define THREAD_SAFE_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue() = default;
    virtual ~ThreadSafeQueue() = default;

    bool empty()
    {
        std::lock_guard lock(m_mutexQueue);
        return m_queue.empty();
    }

    void push(const T& item)
    {
        {
            std::unique_lock lock(m_mutexQueue);
            m_queue.push(item);
        }

        m_cvQueueNotEmpty.notify_one();
    }

    void push(T&& item)
    {
        {
            std::unique_lock lock(m_mutexQueue);
            m_queue.push(std::move(item));
        }

        m_cvQueueNotEmpty.notify_one();
    }

    void push(std::initializer_list<T> initializerList)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutexQueue);
            for (auto& item : initializerList)
            {
                m_queue.push(item);
            }
        }        
        m_cvQueueNotEmpty.notify_all();
    }

    bool try_pop(T& item)
    {
        std::unique_lock<std::mutex> lock(m_mutexQueue, std::try_to_lock);        
        if (!lock.owns_lock() || m_queue.empty() )
        {
            return false;
        }

        item = m_queue.front();
        m_queue.pop();
        return true;
    }

    void pop(T& item)
    {
        std::unique_lock<std::mutex> lock(m_mutexQueue);
        m_cvQueueNotEmpty.wait(lock, [&] { return !m_queue.empty(); });

        item = m_queue.front();
        m_queue.pop();        
    }
private:
    std::queue<T> m_queue;
    std::mutex m_mutexQueue;
    std::condition_variable m_cvQueueNotEmpty;
};

#endif // THREAD_SAFE_QUEUE_HPP

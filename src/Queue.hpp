#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>

template<typename T>
class Queue final
{
public:
   Queue() = default;

   Queue(const Queue&) = default;
   Queue(Queue&&) noexcept = default;

   ~Queue() = default;

   Queue& operator=(const Queue&) = default;
   Queue& operator=(Queue&&) noexcept = default;

   void push(T value);
   T pop();
   std::optional<T> tryPopFor(std::chrono::milliseconds duration);

private:
   std::queue<T> m_queue;

   std::mutex m_mutex;
   std::condition_variable m_cond;
};

template<typename T>
void Queue<T>::push(T value)
{
   {
      std::lock_guard lock{m_mutex};

      m_queue.push(value);
   }

   m_cond.notify_one();
}

template<typename T>
T Queue<T>::pop()
{
   std::unique_lock lock{m_mutex};

   m_cond.wait(lock, [&] { return m_queue.empty() == false; });

   T value = m_queue.front();

   m_queue.pop();

   return value;
}

template<typename T>
std::optional<T> Queue<T>::tryPopFor(std::chrono::milliseconds duration)
{
   std::unique_lock lock{m_mutex};

   if (m_cond.wait_for(lock, duration, [&] { return m_queue.empty() == false; }) == false)
      return std::nullopt;

   T value = m_queue.front();

   m_queue.pop();

   return value;
}

#endif // QUEUE_HPP

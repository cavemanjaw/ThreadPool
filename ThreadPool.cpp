#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <queue>
#include <memory>

class ThreadPool
{
public:

  using Task = std::function<void()>;

  explicit ThreadPool(std::size_t numThreads)
  {
    Start(numThreads);
  }

  ~ThreadPool()
  {
    Stop();
  }

  void Enqueue(Task task)
  {
    {
      std::unique_lock<std::mutex> lock{mEventMutex};
      mTasks.emplace(std::move(task));
    }
  }

private:
  // constexpr parameter/template parameter for reserving place for the number
  // of threads that would be possible to dispatch from the vector?
  std::vector<std::thread> mThreads;

  std::condition_variable mEvent;

  std::mutex mEventMutex;

  // std::atomic<bool>?
  bool mStopping = false;

  std::queue<Task> mTasks;

  void Start(std::size_t numThreads)
  {
    // range-based for loop?
    for (auto threadIter = 0u; threadIter < numThreads; threadIter++)
    {
      mThreads.emplace_back([=]
                             {
        while (true)
        {
          Task task;

          // Additional scope for the mutex to be locked only during critical
          // section. It should not be locked during the task execution.
          {
            std::unique_lock<std::mutex> lock{mEventMutex};
            mEvent.wait(lock, [=]{return mStopping || !mTasks.empty();});
            if (mStopping)
            {
              break;
            }

            task = std::move(mTasks.front());

            // Better function for popping the task?
            // Here it should be just the removal from the queue?
            mTasks.pop();

            // The lock will be freed here, after leaving the scope
          }

          task();
        }
                             });
    }
  }

  void Stop() noexcept
  {
    {
      std::unique_lock<std::mutex> lock{mEventMutex};
      mStopping = true;
    }

    mEvent.notify_all();

    for (auto& thread : mThreads)
    {
      thread.join();
    }
  }
};


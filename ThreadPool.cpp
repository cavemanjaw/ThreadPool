#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>

class ThreadPool
{
public:
  explicit ThreadPool(std::size_t numThreads)
  {
    Start(numThreads);
  }

  ~ThreadPool()
  {
    Stop();
  }

private:
  // constexpr parameter/template parameter for reserving place for the number
  // of threads that would be possible to dispatch from the vector?
  std::vector<std::thread> mThreads;

  std::condition_variable mEvent;

  std::mutex mEventMutex;

  // std::atomic<bool>?
  bool mStopping = false;

  void Start(std::size_t numThreads)
  {
    // range-based for loop?
    for (auto threadIter = 0u; threadIter < numThreads; threadIter++)
    {
      mThreads.emplace_back([=]
                             {
        while (true)
        {
          std::unique_lock<std::mutex> lock{mEventMutex};
          mEvent.wait(lock, [=]{return mStopping;});
          if (mStopping)
          {
            break;
          }
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


//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

#ifndef _XW_THREADS_H_INCLUDED_
#define _XW_THREADS_H_INCLUDED_

//-------------------------------------------------------------------------------

#include <condition_variable>
#include <functional>
#include <vector>
#include <thread>
#include <queue>
#include <future>

//-------------------------------------------------------------------------------

class XWThreadPool {
public:
	using Task = std::function<void()>;

	explicit XWThreadPool(std::size_t numThreads) {
		start(numThreads);
	}

	~XWThreadPool() {
		stop();
	}

	template<class T>
	auto enqueue(T task) ->std::future<decltype(task())>
	{
		auto wrapper = std::make_shared<std::packaged_task<decltype(task())()>>(std::move(task));
		{
			std::unique_lock<std::mutex> lock(mEventMutex);
			mTasks.emplace([=] {
				(*wrapper)();
			});
		}

		mEventVar.notify_one();
		return wrapper->get_future();
	}

private:
	std::vector<std::thread> mThreads;
	std::queue<Task> mTasks;

	std::condition_variable mEventVar;
	std::mutex mEventMutex;
	bool mStopping = false;


	void start(std::size_t numThreads) {
		for (auto i = 0u; i < numThreads; i++) {
			mThreads.emplace_back([=] {
				while (true) {
					Task task;

					{
						std::unique_lock<std::mutex> lock(mEventMutex);

						mEventVar.wait(lock, [=] {return mStopping || !mTasks.empty(); });

						if (mStopping && mTasks.empty()) {
							break;
						}

						task = std::move(mTasks.front());
						mTasks.pop();
					}

					task();
				}
			});
		}
	}

	void stop() noexcept {
		{
			std::unique_lock<std::mutex> lock(mEventMutex);
			mStopping = true;
		}

		mEventVar.notify_all();

		for (auto &thread : mThreads) {
			thread.join();
		}
	}
};

//-------------------------------------------------------------------------------
#endif
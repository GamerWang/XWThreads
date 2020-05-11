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

	void enqueue(Task task) {
		{
			std::unique_lock<std::mutex> lock(mEventMutex);
		}
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
					std::unique_lock<std::mutex> lock(mEventMutex);

					mEventVar.wait(lock, [=] {return mStopping; });

					if (mStopping) {
						break;
					}
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
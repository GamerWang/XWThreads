//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

#ifndef _XW_THREADS_H_INCLUDED_
#define _XW_THREADS_H_INCLUDED_

//-------------------------------------------------------------------------------

#include <condition_variable>
#include <vector>
#include <thread>
#include <queue>
#include <future>

//-------------------------------------------------------------------------------

#include "XWFunction.h"

//-------------------------------------------------------------------------------

#define XW_THREADPOOL_MAIN_EXECUTE true
#define XW_THREADPOOL_MAIN_WAIT false

//-------------------------------------------------------------------------------
namespace XW {
	struct XWTask {
		XWFunction<void()> taskFunc;

		XWTask* prev = NULL;
		XWTask* next = NULL;
	
		XWTask* children = NULL;
	
		void Remove() {
			while (children != NULL) {
				XWTask* c = children->next;
				delete(children);
				children = c;
			}
		}
	};

	void AddTaskToList(XWTask* task, XWTask*& queue) {
		if (task != NULL) {
			task->prev = NULL;
			task->next = queue;
			if (queue != NULL) {
				queue->prev = task;
			}
			queue = task;
		}
	}

	class XWThreadPool {
	public:
		explicit XWThreadPool(std::size_t numThreads) {
			initialize(numThreads);
		}

		~XWThreadPool() {
			stop();
		}

		void start(bool executeMain) {
			{
				std::unique_lock<std::mutex> lock(mExecuteMutex);
				mStart = true;
			}
			mExecuteVar.notify_all();

			for (auto &thread : mThreads) {
				if (executeMain) {
					thread.detach();
				}
				else {
					thread.join();
				}
			}
		}

		template<typename T>
		void addTask(T task) {
			std::unique_lock<std::mutex> lock(mTaskMutex);
			XWTask* newTask = new XWTask();
			newTask->taskFunc = task;
			AddTaskToList(newTask, taskList);
		}

		void addTask(XWTask* task) {
			std::unique_lock<std::mutex> lock(mTaskMutex);
			AddTaskToList(task, taskList);
		}

	private:
		std::vector<std::thread> mThreads;
		XWTask* taskList;

		std::condition_variable mExecuteVar;
		std::mutex mExecuteMutex;
		std::condition_variable mTaskVar;
		std::mutex mTaskMutex;

		std::once_flag mStopFlag;

		bool mStart = false;
		bool mStopping = false;

		void initialize(std::size_t numThreads) {
			for (auto i = 0u; i < numThreads; i++) {
				mThreads.emplace_back([=] {
					while (true) {
						std::unique_lock<std::mutex> executeLock(mExecuteMutex);
						mExecuteVar.wait(executeLock, [=]() {return mStart || mStopping; });

						if (mStopping) {
							break;
						}

						XWTask* currentTask;
						{
							std::unique_lock<std::mutex> taskLock(mTaskMutex);
							mTaskVar.wait(taskLock, [=]() {return taskList != NULL || mStopping; });

							if (mStopping) {
								break;
							}

							currentTask = taskList;
							taskList = currentTask->next;
							if (taskList != NULL) {
								taskList->prev = NULL;
							}
							currentTask->next = NULL;
						}
					
						currentTask->taskFunc();

						XWTask* currentChild = currentTask->children;
						XWTask* nextChild = NULL;
						while (currentChild != NULL) {
							nextChild = currentChild->next;
							addTask(currentChild);
							currentChild = nextChild;
						}

						delete(currentTask);
					
						if (taskList == NULL) {
							std::call_once(mStopFlag, [&]() {
								mStopping = true;
								mTaskVar.notify_all();
							});
							break;
						}
					}
				});
			}
		}

		void stop() noexcept {
			{
				std::unique_lock<std::mutex> executeLock(mExecuteMutex);
				mStopping = true;
			}

			mExecuteVar.notify_all();
			mTaskVar.notify_all();

			while (taskList != NULL) {
				std::unique_lock<std::mutex> taskLock(mTaskMutex);
				XWTask* t = taskList->next;
				taskList->Remove();
				delete(taskList);
				taskList = t;
			}
		}
	};	
}

//-------------------------------------------------------------------------------
#endif
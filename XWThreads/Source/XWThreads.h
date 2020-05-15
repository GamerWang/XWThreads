//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

#ifndef _XW_THREADS_H_INCLUDED_
#define _XW_THREADS_H_INCLUDED_

//-------------------------------------------------------------------------------

#include <condition_variable>
#include <vector>
#include <thread>
#include <queue>

//-------------------------------------------------------------------------------

#include "XWFunction.h"

//-------------------------------------------------------------------------------
namespace XW {
//-------------------------------------------------------------------------------

#define XW_THREADPOOL_MAIN_EXECUTE true
#define XW_THREADPOOL_MAIN_WAIT false
typedef bool MainExecute;

//-------------------------------------------------------------------------------

//! Struct for storing task list & tasks
//!  * needs task function as its work
//!  * can be used as linked list nodes
//!  * can have child tasks which got executed after parent
struct Task {
	Function<void()> taskFunc;		//!< Stores task function

	Task* prev = NULL;
	Task* next = NULL;
	
	Task* children = NULL;			//!< Children tasks tree
	
	//! Release children before Destruct
	void Remove() {
		while (children != NULL) {
			Task* c = children->next;
			delete(children);
			children = c;
		}
	}
};

//! Function for adding a new task to a list
//!  * task will be added to the head of the list
void AddTaskToList(Task* task, Task*& queue) {
	if (task != NULL) {
		task->prev = NULL;
		task->next = queue;
		if (queue != NULL) {
			queue->prev = task;
		}
		queue = task;
	}
}

//-------------------------------------------------------------------------------


//! Class for managing a Thread Pool
class ThreadPool {
public:
	//! Constructor
	explicit ThreadPool(std::size_t numThreads) {initialize(numThreads);}

	//! Destructor
	~ThreadPool() {stop();}

	//! Set Main thread's behavior
	//  * parameter "executable" determins behavior
	//  * XW_THREADPOOL_MAIN_EXECUTE will let main execute simultaneously
	//  * XW_THREADPOOL_MAIN_WAIT will let main wait until all threads finished
	//  * main thread will execute by default
	void holdMain(MainExecute executable);

	//! Launcher after preparation
	void start();

	//! Add Task by function
	template<typename T> void addTask(T task);
	
	//! Add Task by task pointer
	void addTask(Task* task);

private:
	std::vector<std::thread> mThreads;		//!< Stores threads
	std::vector<bool> mThreadDone;			//!< Stores threads done symbol
	Task* taskList;							//!< Stores task tree

	std::mutex mExecuteMutex;				//!< Locks threads before start
	std::condition_variable mExecuteVar;	//!< & hold main thread

	std::mutex mTaskMutex;					//!< Locks threads when add & get tasks
	std::condition_variable mTaskVar;		//!< & hold thread when task list empty

	std::mutex mDoneMutex;

	std::mutex mWorkingMutex;
	int runningThreads = 0;

	std::once_flag mStopFlag;				//!< stop when no task can be ger from list

	bool mStart = false;
	bool mStopping = false;

	//! Generate pool threads
	void initialize(std::size_t numThreads);

	//! Terminate all threads in pool
	void stop() noexcept;

	//! Check if all threads finished work
	//  * return true when all threads terminated or no thread in pool
	bool allThreadsDone();
};

//-------------------------------------------------------------------------------
// Implementation of ThreadPool
//-------------------------------------------------------------------------------

void ThreadPool::holdMain(MainExecute executable) {
	if (!executable) {
		std::unique_lock<std::mutex> lock(mExecuteMutex);
		mExecuteVar.wait(lock, [=]() {return mStopping; });
	}
}

void ThreadPool::start() {
	{
		std::unique_lock<std::mutex> lock(mExecuteMutex);
		mStart = true;
	}
	mExecuteVar.notify_all();
}

template<typename T>
void ThreadPool::addTask(T task) {
	std::unique_lock<std::mutex> lock(mTaskMutex);
	Task* newTask = new Task();
	newTask->taskFunc = task;
	AddTaskToList(newTask, taskList);
	mTaskVar.notify_all();
}

void ThreadPool::addTask(Task* task) {
	std::unique_lock<std::mutex> lock(mTaskMutex);
	AddTaskToList(task, taskList);
	mTaskVar.notify_all();
}

void ThreadPool::initialize(std::size_t numThreads) {
	runningThreads = 0;

	mThreads.clear();
	mThreadDone.clear();
	mThreads.reserve(numThreads);
	mThreadDone.reserve(numThreads);
	for (auto i = 0u; i < numThreads; i++) {
		mThreadDone.emplace_back(false);
		mThreads.emplace_back([=] {
			
			{
				std::unique_lock<std::mutex> executeLock(mExecuteMutex);
				mExecuteVar.wait(executeLock, [=]() {return mStart || mStopping; });
			}

			while (true) {
				Task* currentTask;
				
				{
					std::unique_lock<std::mutex> taskLock(mTaskMutex);
					mTaskVar.wait(taskLock, [=]() {return taskList != NULL || mStopping; });

					if (mStopping) {
						break;
					}

					{
						mWorkingMutex.lock();
						runningThreads++;
						mWorkingMutex.unlock();
					}

					currentTask = taskList;
					taskList = currentTask->next;
					if (taskList != NULL) {
						taskList->prev = NULL;
					}
					currentTask->next = NULL;
				}

				currentTask->taskFunc();

				Task* currentChild = currentTask->children;
				Task* nextChild = NULL;
				while (currentChild != NULL) {
					nextChild = currentChild->next;
					addTask(currentChild);
					currentChild = nextChild;
				}

				delete(currentTask);

				{	
					mWorkingMutex.lock();
					runningThreads--;
					mWorkingMutex.unlock();
				}

				{
					mWorkingMutex.lock();
					if (taskList == NULL && runningThreads <= 0) {
						mWorkingMutex.unlock();
						std::call_once(mStopFlag, [&]() {
							mStopping = true;
							mTaskVar.notify_all();
						});
						break;
					}
					mWorkingMutex.unlock();
				}
			}

			{
				std::unique_lock<std::mutex> doneLock(mDoneMutex);
				mThreadDone[i] = true;
				if (allThreadsDone()) {
					mExecuteVar.notify_all();
				}
			}
		});
	}
	for (auto &thread : mThreads) {
		thread.detach();
	}
}

void ThreadPool::stop() noexcept {
	{
		std::unique_lock<std::mutex> executeLock(mExecuteMutex);
		mStopping = true;
	}

	mExecuteVar.notify_all();
	mTaskVar.notify_all();

	{
		std::unique_lock<std::mutex> doneLock(mDoneMutex);
		if (allThreadsDone()) {
			mExecuteVar.notify_all();
		}
	}

	while (taskList != NULL) {
		std::unique_lock<std::mutex> taskLock(mTaskMutex);
		Task* t = taskList->next;
		taskList->Remove();
		delete(taskList);
		taskList = t;
	}
}

bool ThreadPool::allThreadsDone() {
	bool done = true;
	if (!mThreadDone.empty()) {
		for (auto t : mThreadDone) {
			done = done & t;
		}
	}
	return done;
}

//-------------------------------------------------------------------------------
} // namespace XW
//-------------------------------------------------------------------------------
#endif
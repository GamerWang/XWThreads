#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <ctime>

#include <iostream>

#include "Source/XWThreads.h"

using namespace XW;

//! Test Tasks
//! All supports operator()
class funcTest1 {
public:
	void operator()() {
		printf("Task1 start\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("Task1 end\n");
	}
};

struct funcTest2 {
	void operator()() {
		printf("Task2 start\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		printf("Task2 end\n");
	}
};

void funcTest3() {
	printf("Task3 start\n");
	printf("Task3 end\n");
}

auto funcTest4 = [] {
	printf("Task4 start\n");
	printf("Task4 end\n");
};

int main()
{
	{
		//! Initialize pool with 50 threads
		ThreadPool pool{ 50 };

		//! Make first task pointer
		Task* x = new Task();
		x->taskFunc = funcTest1();
		
		//! Make 2 new tasks
		Task* xChild1 = new Task();
		xChild1->taskFunc = funcTest3;
		Task* xChild2 = new Task();
		xChild2->taskFunc = funcTest4;

		//! Assign new tasks as child tasks
		AddTaskToList(xChild1, x->children);
		AddTaskToList(xChild2, x->children);

		//! Add tasks to pool's task list
		pool.addTask(x);				//!< add task by Task pointer
		pool.addTask(funcTest2());		//!< add task by task function

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));	//!< holds main thread, all side threads sleep
		
		//! All side threads will start executing after this start call
		pool.start();
		
		//! Determins main threads behavior
		//	* if pass in XW_THREADPOOL_MAIN_EXECUTE or not calling this function, main will execute simultaneously
		//  * if pass in XW_THREADPOOL_MAIN_WAIT, main thread will sleep until all threads finished
		pool.holdMain(XW_THREADPOOL_MAIN_WAIT);

		printf("Main thread done\n");	//!< Main thread's work
	}
	
	return 0;
}

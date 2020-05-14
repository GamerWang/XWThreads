#include <iostream>
#include <chrono>

#include "Source/XWThreads.h"

using namespace XW;

// test tasks
class funcTest1 {
public:
	void operator()() {
		std::cout << "Inside funcTest1" << std::endl;
	}
};

struct funcTest2 {
	void operator()() {
		std::cout << "Inside funcTest2" << std::endl;
	}
};

void funcTest3() {
	std::cout << "Inside funcTest3" << std::endl;
}

auto funcTest4 = [] {
	std::cout << "Inside funcTest4" << std::endl; 
};

int main()
{
	{
		ThreadPool pool{ 2 };
		//ThreadPool pool(12);

		Task* x = new Task();
		x->taskFunc = funcTest1();
		
		Task* xChild1 = new Task();
		xChild1->taskFunc = funcTest3;
		Task* xChild2 = new Task();
		xChild2->taskFunc = funcTest4;

		AddTaskToList(xChild1, x->children);
		AddTaskToList(xChild2, x->children);

		pool.addTask(x);
		pool.addTask(funcTest2());

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		pool.start();
		pool.holdMain(XW_THREADPOOL_MAIN_WAIT);
		std::cout << "Main thread function execute" << std::endl;
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		pool.~ThreadPool();
	}
	
	return 0;
}

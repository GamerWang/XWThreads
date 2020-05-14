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

		XWThreadPool pool{ 2 };

		XWTask* x = new XWTask();
		x->taskFunc = funcTest1();
		
		XWTask* xChild1 = new XWTask();
		xChild1->taskFunc = funcTest3;
		XWTask* xChild2 = new XWTask();
		xChild2->taskFunc = funcTest4;

		AddTaskToList(xChild1, x->children);
		AddTaskToList(xChild2, x->children);

		pool.addTask(x);
		pool.addTask(funcTest2());

		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		pool.start(XW_THREADPOOL_MAIN_EXECUTE);
		//std::cout << "Main thread function execute" << std::endl;
		pool.~XWThreadPool();
	}
	
	return 0;
}

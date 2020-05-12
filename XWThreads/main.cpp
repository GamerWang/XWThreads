#include <iostream>

#include "Source/XWThreads.h"

int main()
{
	{
		XWThreadPool pool{ 16 };

		auto f1 = pool.enqueue([] {
			return 1;
		});

		auto f2 = pool.enqueue([] {
			return 2;
		});

		std::cout << (f1.get() + f2.get()) << std::endl;
	}
	
	return 0;
}

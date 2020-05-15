# XWThreads
A C++11 Thread Pool implementation. User can generate a pool with expected worker threads and set tasks for workers to do.
User can set main thread to wait for all worker threads to finish or execute simultaneously.
User can also set child tasks, all child tasks will be executed only when their parent task is done.

## Usage

Get code:

```
git clone https://github.com/GamerWang/XWThreads.git
```

then include XWThreads.h and XWFunction.h in your project

Basic usage:
```c++
// Create thread pool with 10 worker threads
// All workers are waiting
XW::ThreadPool pool{10}

// Example task functions with operator() implemented
class func1 {public:void operator()() {}};
struct func2 {void operator()() {}};
void func3() {}
auto func4 = [] {};

// Add tasks by task function
pool.addTask(func1());
pool.addTask(func2());

// Create a task with child task
XW::Task* x = new XW::Task();
x->taskFunc = func3;
XW::Task* xChild = new XW::Task();
xChild->taskFunc = func4;
XW::AddTaskToList(xChild, x->children);

// Add task by Task pointer
pool.addTask(x);

// Wake up all worker threads
pool.start();

// Set main thread to wait for all workers done
pool.holdMain(XW_THREADPOOL_MAIN_WAIT);

```

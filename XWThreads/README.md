# XWThreads
A C++11 Thread Pool implementation. User can generate a pool with expected worker threads and set tasks for workers to do.
User can set main thread to wait for all worker threads to finish or execute simultaneously.
User can also set child tasks, all child tasks will be executed only when their parent task is done.

Example Demo code can be found under [XWThreads/Demo/main.cpp](https://github.com/GamerWang/XWThreads/blob/master/XWThreads/Demo/main.cpp)

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

## API
### XW::Task
|Member|Description|Example Usage|
|-------------|-------------|-------------|
|Function<void()> taskFunc|Stores task function|Task* x = new Task();<br>x->taskFunc = [] {};<br>x->taskFunc();|
|Task* prev|Linked list previous node pointer||
|Task* next||Linked list next node pointer||
|Task* children|Child Task tree head pointer||
|void Remove()|Free child tree memory||

### XW::AddTaskToList
|Member|Description|Example Usage|
|-------------|-------------|-------------|
|void AddTaskToList(Task* task, Task*& queue)|Adds one Task node to the head of the queue|Task* x = new Task();<br>x->taskFunc = [] {};<br>Task* xChild = new Task();<br>xChild->taskFunc=[]{}<br>AddTaskToList(xChild, x->children);|

### XW::ThreadPool
|Member|Description|Example Usage|
|-------------|-------------|-------------|
|explicit ThreadPool(std::size_t numThreads)|Constructor function|ThreadPool pool1{10};<br>ThreadPool pool2(5);|
|void holdMain(MainExecute executable)|Set main thread's behavior<br>parameter "executable" determins behavior<br>"XW_THREADPOOL_MAIN_EXECUTE" will let main execute simultaneously<br>"XW_THREADPOOL_MAIN_WAIT" will let main wait until all threads finished<br>main thread will execute by default|pool.holdMain(XW_THREADPOOL_MAIN_WAIT);|
|void start()|Launcher after preparation, wake up all worker threads||
|template<typename T> void addTask(T task);|Add task by function, can take in any type supports operator()||
|void addTask(Task* task)|Add task by Task pointer||

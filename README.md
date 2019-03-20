# C++ thread pool
A simple, header-only thread pool implementation in C++

# Getting Started
To use the thread pool in your project, you simply include the header in your project.

You also have to add the pthread library during the linking step, since the thread pool uses POSIX threads.

# Example
```c++
#include <iostream>
#include "thread_pool.h"

using namespace std;

// Extending the base thread_pool::task class to provide void run() implementation.
class my_task : public thread_pool::task {
    private:
        int i;

    public:
        my_task(int i) : i(i) {}
        
        void run() {
            cout << "Hello from thread, i = " << i << endl;
        }
};

int main() {
    // Creating a thread pool with 4 threads.
    thread_pool thread_pool(4);

    // Adding jobs in the thread pool.
    for (int i = 0; i < 50; i++)
        thread_pool.add_task(new my_task(i));

    // Thread pool destructor will wait here, until all jobs are completed.
}
```

# License
This project is licensed under the MIT License - see the LICENSE file for details

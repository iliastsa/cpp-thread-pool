#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <cstdlib>

class thread_pool {
    public:
        class task {
            public:
                virtual ~task() {}
                virtual void run() = 0;
        };
    private:
        class task_queue {
            private:
                struct task_queue_node {
                    class task* task;
                    task_queue_node *next;

                    task_queue_node(class task* task) : task(task), next(nullptr) {}
                };

                task_queue_node *head;
                task_queue_node *tail;

                // Task queue synchronization;
                pthread_mutex_t queue_rwlock;
                pthread_cond_t  queue_available;

                // Empty queue synchronization;
                pthread_cond_t  queue_empty;

            public:
                task_queue() : head(nullptr), tail(nullptr), n_tasks(0) {
                    // TODO: Error checking
                    pthread_cond_init(&queue_available, NULL);
                    pthread_mutex_init(&queue_rwlock, NULL);

                    pthread_cond_init(&queue_empty, NULL);
                }

                ~task_queue(){
                    pthread_mutex_destroy(&queue_rwlock);
                    pthread_cond_destroy(&queue_available);
                    pthread_cond_destroy(&queue_empty);

                    while (head != nullptr) {
                        task_queue_node* next = head->next;

                        delete head;
                        head = next;
                    }
                }

                int n_tasks;

                void lock() {
                    pthread_mutex_lock(&queue_rwlock);
                }

                void unlock() {
                    pthread_mutex_unlock(&queue_rwlock);
                }

                void wait_empty() {
                    pthread_cond_wait(&queue_empty, &queue_rwlock);
                }

                void wait() {
                    pthread_cond_wait(&queue_available, &queue_rwlock);
                }

                void broadcast() {
                    pthread_cond_broadcast(&queue_available);
                }

                void broadcast_queue_empty() {
                    pthread_cond_broadcast(&queue_empty);
                }

                void insert(task* task) {
                    task_queue_node *new_node = new task_queue_node(task);

                    // TODO: Error checking
                    pthread_mutex_lock(&queue_rwlock);

                    // Insert into queue
                    if (n_tasks == 0) {
                        head = new_node;
                        tail = new_node;
                    } else {
                        tail->next = new_node;
                        tail       = new_node;
                    }

                    n_tasks++;

                    // Signal any thread waiting to read from the queue.
                    pthread_cond_signal(&queue_available);

                    // Unlock the lock.
                    pthread_mutex_unlock(&queue_rwlock);
                }

                thread_pool::task* next() {
                    if (n_tasks == 0)
                        return nullptr;

                    task* ret = head->task;
                    task_queue_node* next = head->next;

                    delete head;
                    n_tasks--;

                    head = next;

                    return ret;
                }
        } task_queue;

        pthread_t *threads;

        volatile int running;
        volatile int n_threads;
        volatile int active;

        static void* thread_run(void *t_pool) {
            thread_pool* pool = (thread_pool*) t_pool;

            for (;;) {
                // Acquire lock since the get function requires it
                pool->task_queue.lock();

                while (pool->task_queue.n_tasks == 0 && pool->running == 1)
                    pool->task_queue.wait();

                if (pool->running == 0 && pool->task_queue.n_tasks == 0)
                    break;

                task* task = pool->task_queue.next();

                pool->active++;

                pool->task_queue.unlock();

                task->run();

                delete task;

                pool->task_queue.lock();

                pool->active--;

                if (pool->active == 0 && pool->task_queue.n_tasks == 0)
                    pool->task_queue.broadcast_queue_empty();

                pool->task_queue.unlock();
            }

            pool->task_queue.unlock();

            pthread_exit(NULL);

            return NULL;
        }

    public:
        thread_pool(int n_threads) : running(1), n_threads(n_threads), active(0) {
            threads = new pthread_t[n_threads];

            // TODO: Error checking

            // Initialize threads
            for (int i = 0; i < n_threads; ++i)
                pthread_create(threads + i, NULL, thread_run, this);
        }

        ~thread_pool() {
            task_queue.lock();

            running = 0;

            task_queue.broadcast();

            task_queue.unlock();

            for (int i = 0; i < n_threads; ++i)
                pthread_join(threads[i], NULL);

            delete[] threads;
        }

        void add_task(task* task) {
            task_queue.insert(task);
        }

        void wait_all() {
            task_queue.lock();

            while (task_queue.n_tasks != 0 || active != 0)
                task_queue.wait_empty();

            task_queue.unlock();
        }
};

#endif

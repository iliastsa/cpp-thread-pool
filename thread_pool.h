#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>

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
                task_queue();
                ~task_queue();

                int n_tasks;

                void lock();
                void unlock();
                void wait();
                void broadcast();
                void broadcast_queue_empty();

                void insert(task* task);
                task* next();
        } task_queue;

        pthread_t *threads;

        volatile int running;
        volatile int n_threads;
        volatile int active;

        static void* thread_run(void *t_pool);

    public:
        thread_pool(int n_workers);
        ~thread_pool();

        void add_task(task* task);
        void wait_all();
};

#endif

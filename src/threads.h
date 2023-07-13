
#pragma once

#include <pthread.h>
#include <stdatomic.h>
#include "vector.h"


typedef struct {
    pthread_t thread;
    _Atomic(char) stopping;
    pthread_mutex_t done_lock;
    pthread_cond_t done_notify;
    pthread_mutex_t dotask_lock;
    pthread_cond_t dotask_notify;
    void (*task)(void*);
    void* task_args;
} Thread;

typedef struct {
    Vector threads;
    pthread_mutex_t lock;
} ThreadPool;


ThreadPool create_thread_pool();

size_t threadpool_do(ThreadPool* pool, void (*task)(void*), void* args);

void threadpool_await_task(ThreadPool* pool, size_t task);

void threadpool_cleanup(ThreadPool* pool);
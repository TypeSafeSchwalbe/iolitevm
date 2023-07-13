
#include <stdio.h>
#include "threads.h"


ThreadPool create_thread_pool() {
    ThreadPool tp;
    tp.threads = create_vector(sizeof(Thread));
    pthread_mutex_init(&tp.lock, NULL);
    return tp;
}

void* threadpool_worker(void* args) {
    Thread* thread = (Thread*) args;

    for(;;) {
        // wait for a task
        pthread_mutex_lock(&thread->dotask_lock);
        while(thread->task == NULL && !thread->stopping) {
            pthread_cond_wait(&thread->dotask_notify, &thread->dotask_lock);
        }
        if(thread->stopping) {
            pthread_mutex_unlock(&thread->dotask_lock);
            return NULL;
        }
        // perform the task
        thread->task(thread->task_args);
        thread->task = NULL;
        thread->task_args = NULL;
        pthread_mutex_lock(&thread->done_lock);
        pthread_cond_broadcast(&thread->done_notify);
        pthread_mutex_unlock(&thread->done_lock);
        pthread_mutex_unlock(&thread->dotask_lock);
    }

    return NULL;
}

size_t threadpool_do(ThreadPool* tp, void (*task_ptr)(void*), void* args_ptr) {
    // look for an idle thread in the pool and use it
    pthread_mutex_lock(&tp->lock);
    for(size_t task = 0; task < tp->threads.size; task += 1) {
        Thread* thread = vector_get(&tp->threads, task);
        if(pthread_mutex_trylock(&thread->dotask_lock) != 0) { continue; }
        if(thread->task != NULL) {
            pthread_mutex_unlock(&thread->dotask_lock);
            continue;
        }
        thread->task = task_ptr;
        thread->task_args = args_ptr;
        pthread_cond_broadcast(&thread->dotask_notify);
        pthread_mutex_unlock(&thread->dotask_lock);
        pthread_mutex_unlock(&tp->lock);
        return task;
    }
    // increase pool size
    Thread new_thread;
    vector_push(&tp->threads, &new_thread);
    Thread* thread = vector_get(&tp->threads, tp->threads.size - 1);
    pthread_mutex_init(&thread->dotask_lock, NULL);
    pthread_cond_init(&thread->dotask_notify, NULL);
    pthread_mutex_init(&thread->done_lock, NULL);
    pthread_cond_init(&thread->done_notify, NULL);
    thread->stopping = 0;
    thread->task = task_ptr;
    thread->task_args = args_ptr;
    thread->thread = pthread_create(&thread->thread, NULL, threadpool_worker, thread);
    pthread_cond_broadcast(&thread->dotask_notify);
    pthread_mutex_unlock(&thread->dotask_lock);
    pthread_mutex_unlock(&tp->lock);
    return tp->threads.size - 1;
}

void threadpool_await_task(ThreadPool* tp, size_t task) {
    Thread* thread = vector_get(&tp->threads, task);
    if(thread->task == NULL) { return; }
    pthread_mutex_lock(&thread->done_lock);
    pthread_cond_wait(&thread->done_notify, &thread->done_lock);
    pthread_mutex_unlock(&thread->done_lock);
}

void threadpool_cleanup(ThreadPool* tp) {
    for(size_t task = 0; task < tp->threads.size; task += 1) {
        Thread* thread = vector_get(&tp->threads, task);
        pthread_mutex_lock(&thread->dotask_lock);
        thread->stopping = 1;
        pthread_cond_broadcast(&thread->dotask_notify);
        pthread_mutex_unlock(&thread->dotask_lock);
        pthread_mutex_destroy(&thread->dotask_lock);
        pthread_cond_destroy(&thread->dotask_notify);
        pthread_mutex_destroy(&thread->done_lock);
        pthread_cond_destroy(&thread->done_notify);
    }
    pthread_mutex_destroy(&tp->lock);
    vector_cleanup(&tp->threads);
}